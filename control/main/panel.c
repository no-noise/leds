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

#include <assert.h>
#include <driver/gpio.h>
#include <driver/i2s.h>
#include <esp32/rom/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/gpio_sig_map.h>
#include <soc/i2s_struct.h>
#include <stdbool.h>
#include <stdint.h>

#include <util.h>
#include <warnings.h>

// --- Types and constants -----------------------------------------------------

#define N_DMA_BUFS 2
#define DMA_BUF_LEN 1024
#define SAMPLE_RATE 10000000

// --- Macros and inline functions ---------------------------------------------

// --- Globals -----------------------------------------------------------------

// --- Helper declarations -----------------------------------------------------

// --- API ---------------------------------------------------------------------

void panel_init(void)
{
}

void panel_test_pattern(uint32_t gpio_no_1, uint32_t gpio_no_2)
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
        .tx_desc_auto_clear = false, // broken - see workaround below.
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

    uint32_t ticks_pause = 1000 / portTICK_PERIOD_MS;
    uint32_t iter = 0;

    // Create 1 ms's worth of output (= 10000 samples). Make gpio_no_1 flip
    // every 100 ns, gpio_no_2 every 200 ns.

    static uint16_t data[SAMPLE_RATE / 1000];

    for (int32_t i = 0; i < SAMPLE_RATE / 1000; ++i) {
        data[i] = (uint16_t)(i & 3);
    }

    // Enough to fill all DMA buffers: # buffers * # samples * # channels.
    static uint16_t silence[N_DMA_BUFS * DMA_BUF_LEN * 2] = { 0 };

    while (true) {
        printf("%d\n", iter++);

        size_t written;
        i2s_write(0, data, sizeof data, &written, 100);
        assert(written == sizeof data);

        // Workaround for broken tx_desc_auto_clear: fill all DMA buffers with
        // silence.
        i2s_write(0, silence, sizeof silence, &written, 100);
        assert(written == sizeof silence);

        vTaskDelay(ticks_pause);
    }
}

// --- Helpers -----------------------------------------------------------------
