// control.c
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

#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <panel.h>
#include <util.h>
#include <wifi.h>

#include <warnings.h>

// --- Types and constants -----------------------------------------------------

#define GPIO_NO_1 4
#define GPIO_NO_2 5

// --- Macros and inline functions ---------------------------------------------

// --- Globals -----------------------------------------------------------------

// --- Helper declarations -----------------------------------------------------

// --- API ---------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"

void app_main(void)
{
    ESP_LOGI("NN", "no noise controller");

    util_init();

    util_never_fails(nvs_flash_init);
    util_never_fails(esp_event_loop_create_default);

    panel_init(GPIO_NO_1, GPIO_NO_2);
    wifi_init();

    panel_test_pattern();
}

#pragma GCC diagnostic pop

// --- Helpers -----------------------------------------------------------------
