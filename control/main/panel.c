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

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdint.h>

#include <util.h>
#include <warnings.h>

// --- Types -------------------------------------------------------------------

// --- Constants and macros ----------------------------------------------------

// --- Globals -----------------------------------------------------------------

// --- Helper declarations -----------------------------------------------------

// --- API ---------------------------------------------------------------------

void panel_init(void)
{
}

void panel_test_pattern(int32_t gpio_no)
{
    gpio_config_t conf = {
        .pin_bit_mask = (uint64_t)1 << gpio_no,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&conf);

    uint32_t cycles_1000 = util_ns_to_cycles(1000);
    uint32_t cycles_2000 = util_ns_to_cycles(2000);
    uint32_t cycles_3000 = util_ns_to_cycles(3000);

    uint32_t ticks_pause = 1000 / portTICK_PERIOD_MS;

    uint32_t iter = 0;

    while (true) {
        printf("%d\n", iter++);

        // Disable interrupts on current core.
        util_enter_critical();

        // Level 1 for 1 microsecond.
        gpio_set_level(gpio_no, 1);
        util_delay_in_critical(cycles_1000);

        // Level 0 for 2 microseconds.
        gpio_set_level(gpio_no, 0);
        util_delay_in_critical(cycles_2000);

        // Level 1 for 3 microseconds.
        gpio_set_level(gpio_no, 1);
        util_delay_in_critical(cycles_3000);

        // Level 0.
        gpio_set_level(gpio_no, 0);

        // Re-enable interrupts on current core.
        util_leave_critical();

        // Pause for 1 second.
        vTaskDelay(ticks_pause);
    }
}

// --- Helpers -----------------------------------------------------------------
