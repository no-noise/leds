// util.h
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

#include <esp_err.h>
#include <stdint.h>
#include <stdlib.h>
#include <xtensa/core-macros.h>

// --- Types and constants -----------------------------------------------------

// --- Macros and inline functions ---------------------------------------------

// --- Globals -----------------------------------------------------------------

// --- API ---------------------------------------------------------------------

// Initialize.
void util_init(void);

// Get current CPU cycle count.
static inline uint32_t util_cycle_count(void)
{
    return (uint32_t)XTHAL_GET_CCOUNT();
}

// Convert the given number of nanoseconds to CPU cycles.
uint32_t util_ns_to_cycles(uint32_t ns);

// Disable interrupts on the current core.
void util_enter_critical(void);

// Enable interrupts on the current core.
void util_leave_critical(void);

// Busy-wait for the given number of CPU cycles. Interrupts must be disabled -
// i.e., util_enter_critical() must have been called - before calling this.
static inline void util_delay_in_critical(uint32_t start, uint32_t cycles)
{
    while ((uint32_t)XTHAL_GET_CCOUNT() - start < cycles) {
        // do nothing
    }
}

void util_esp_error(const char *func, esp_err_t err);

#define util_failed(func, ...) ({      \
    esp_err_t err = func(__VA_ARGS__); \
                                       \
    if (err != ESP_OK) {               \
        util_esp_error(#func, err);    \
    }                                  \
                                       \
    err != ESP_OK;                     \
})

#define util_never_fails(func, ...) do { \
    esp_err_t err = func(__VA_ARGS__);   \
                                         \
    if (err != ESP_OK) {                 \
        util_esp_error(#func, err);      \
        abort();                         \
    }                                    \
} while(0);
