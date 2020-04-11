// panel.c
//
// Copyright (C) 2020 by the contributors
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.

// --- Includes ----------------------------------------------------------------

#include <panel.h>

#include <freertos/FreeRTOS.h> // pre 4.1, IDF headers depend on these two
#include <freertos/task.h>

#include <assert.h>
#include <driver/gpio.h>
#include <driver/i2s.h>
#include <esp32/rom/gpio.h>
#include <esp32/rom/lldesc.h>
#include <soc/gpio_sig_map.h>
#include <soc/i2s_struct.h>
#include <stdbool.h>
#include <stdint.h>

#include <util.h>
#include <warnings.h>

// --- Types and constants -----------------------------------------------------

#define N_DMA_BUFS 2
#define N_CHANNELS 2
#define DMA_BUF_LEN 1024
#define DMA_BUF_SZ (DMA_BUF_LEN * N_CHANNELS * sizeof (uint16_t))
#define SAMPLE_RATE 10000000

// --- Macros and inline functions ---------------------------------------------

// --- Globals -----------------------------------------------------------------

// --- Helper declarations -----------------------------------------------------

static void write_data(const void *data, size_t sz);
static volatile uint8_t *get_dma_buffer(void);
static void v_memcpy(volatile void *to, const void *from, size_t sz);
static void v_memset(volatile void *to, uint8_t val, size_t sz);

// --- API ---------------------------------------------------------------------

void panel_init(uint32_t gpio_no_1, uint32_t gpio_no_2)
{
    // Completely normal GPIO setup.

    gpio_config_t gpio_conf = {
        .pin_bit_mask =
            (uint64_t)1 << gpio_no_1 |
            (uint64_t)1 << gpio_no_2,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&gpio_conf);

    // Almost normal I2S setup. Note the sample rate, though.

    i2s_config_t i2s_conf = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        // Divide by 16. I assume that we need to do this, because we'll switch
        // to LCD mode, which transmits 16 bits in parallel. This setting gives
        // us 100 ns per sample.
        .sample_rate = SAMPLE_RATE / 16,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_PCM,
        .intr_alloc_flags = 0,
        .dma_buf_count = N_DMA_BUFS,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_driver_install(0, &i2s_conf, 0, NULL);

    // In LCD mode, the 16-bit samples are output via signals I2S0O_DATA_OUT8
    // through I2S0O_DATA_OUT23. Instead of using i2s_set_pin(), we manually
    // connect bits 0 and 1 of the 16-bit samples to gpio_no_1 and gpio_no_2.

    gpio_matrix_out(gpio_no_1, I2S0O_DATA_OUT8_IDX, false, false);
    gpio_matrix_out(gpio_no_2, I2S0O_DATA_OUT9_IDX, false, false);

    // Write directly to I2S_CONF2_REG to enable LCD mode.
    I2S0.conf2.lcd_en = 1;
}

void panel_test_pattern(void)
{
    // Create 1 ms's worth of output (= 10000 samples). Make gpio_no_1 flip
    // every 100 ns, gpio_no_2 every 200 ns.

    static uint16_t samples[SAMPLE_RATE / 1000];

    for (int32_t i = 0; i < SAMPLE_RATE / 1000; ++i) {
        samples[i] = (uint16_t)(i & 3);
    }

    // Output the samples once per second.

    uint32_t ticks_pause = 1000 / portTICK_PERIOD_MS;
    uint32_t iter = 0;

    while (true) {
        printf("%d\n", iter++);

        write_data(samples, sizeof samples);

        vTaskDelay(ticks_pause);
    }
}

// --- Helpers -----------------------------------------------------------------

static void write_data(const void *data, size_t sz)
{
    assert((sz & 3) == 0);

    // Copy data to DMA buffers as they become available. Pad data with
    // silence to make its length a multiple of DMA_BUF_SZ.

    const uint8_t *data_8 = data;

    while (true) {
        // Wait for a DMA buffer to become available.
        volatile uint8_t *buf = get_dma_buffer();

        if (sz > DMA_BUF_SZ) {
            // Fill DMA buffer with samples.
            v_memcpy(buf, data_8, DMA_BUF_SZ);

            data_8 += DMA_BUF_SZ;
            sz -= DMA_BUF_SZ;
        }
        else {
            // Fill DMA buffer with samples.
            v_memcpy(buf, data_8, sz);
            // Pad DMA buffer with silence.
            v_memset(buf + sz, 0, DMA_BUF_SZ - sz);
            break;
        }
    }

    // Done with copying samples. Now fill DMA buffers with silence as they
    // become available.

    for (int32_t i = 0; i < N_DMA_BUFS; ++i) {
        // Wait for a DMA buffer to become available.
        volatile uint8_t *buf = get_dma_buffer();

        // Fill it with silence.
        v_memset(buf, 0, DMA_BUF_SZ);
    }
}

static volatile uint8_t *get_dma_buffer(void)
{
    lldesc_t *prev_desc = (lldesc_t *)I2S0.out_eof_des_addr;
    lldesc_t *desc;

    // Wait for the next DMA descriptor to finish.

    do {
        desc = (lldesc_t *)I2S0.out_eof_des_addr;
    } while (desc == prev_desc);

    // Return the DMA buffer of the DMA descriptor that just finished.
    return desc->buf;
}

static void v_memcpy(volatile void *to, const void *from, size_t sz)
{
    assert((sz & 3) == 0);

    volatile uint32_t *to_32 = to;
    volatile uint32_t *end_32 = to_32 + (sz >> 2);

    const uint32_t *from_32 = from;

    while (to_32 < end_32) {
        *to_32++ = *from_32++;
    }
}

static void v_memset(volatile void *to, uint8_t val, size_t sz)
{
    assert((sz & 3) == 0);

    volatile uint32_t *to_32 = to;
    volatile uint32_t *end_32 = to_32 + (sz >> 2);

    while (to_32 < end_32) {
        *to_32++ = val;
    }
}
