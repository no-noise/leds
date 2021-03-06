// util.c
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

#include <util.h>

#include <freertos/FreeRTOS.h> // pre 4.1, IDF headers depend on this

#include <assert.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_spi_flash.h>
#include <esp_system.h>
#include <esp32/clk.h>
#include <stdbool.h>
#include <stdint.h>

#include <warnings.h>

// --- Types and constants -----------------------------------------------------

// --- Macros and inline functions ---------------------------------------------

// --- Globals -----------------------------------------------------------------

static int32_t g_freq_mhz;

// --- Helper declarations -----------------------------------------------------

static void output_banner(const esp_chip_info_t *info);

// --- API ---------------------------------------------------------------------

void util_init(void)
{
    int32_t freq = esp_clk_cpu_freq();

    g_freq_mhz = freq / 1000000;
    assert(g_freq_mhz * 1000000 == freq);

    esp_chip_info_t info;
    esp_chip_info(&info);

    output_banner(&info);
}

uint32_t util_ns_to_cycles(uint32_t ns)
{
    assert(ns <= 1000000);

    return ns * (uint32_t)g_freq_mhz / 1000;
}

void util_enter_critical(void)
{
    portDISABLE_INTERRUPTS();
}

void util_leave_critical(void)
{
    portENABLE_INTERRUPTS();
}

void util_esp_error(const char *func, esp_err_t err)
{
    ESP_LOGE("NN", "%s() failed: %d (%s)", func, err, esp_err_to_name(err));
}

// --- Helpers -----------------------------------------------------------------

static void output_banner(const esp_chip_info_t *info)
{
    const char *model = info->model == CHIP_ESP32 ? "esp32" : "esp32-s2";
    int32_t revision = info->revision;
    int32_t n_cores = info->cores;

    const char *flash_type = (info->features & CHIP_FEATURE_EMB_FLASH) == 0 ?
            "ext" : "int";
    size_t flash_sz = spi_flash_get_chip_size();

    size_t heap_sz = esp_get_free_heap_size();

    ESP_LOGI("NN", "%s rev %d cores %d freq %d flash-%s %zu heap %zu",
            model, revision, n_cores, g_freq_mhz, flash_type, flash_sz,
            heap_sz);
}
