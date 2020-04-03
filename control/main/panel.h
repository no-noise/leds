// panel.h
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

#include <stdint.h>

// --- Types -------------------------------------------------------------------

// --- Constants and macros ----------------------------------------------------

// --- Globals -----------------------------------------------------------------

// --- API ---------------------------------------------------------------------

// Initialize.
void panel_init(uint32_t gpio_no_1, uint32_t gpio_no_2);

// Generate test pattern.
void panel_test_pattern(void);
