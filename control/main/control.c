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

#include <stdio.h>

#include <panel.h>
#include <util.h>
#include <warnings.h>

// --- Types -------------------------------------------------------------------

// --- Constants and macros ----------------------------------------------------

#define GPIO_NO_1 4
#define GPIO_NO_2 5

// --- Globals -----------------------------------------------------------------

// --- Helper declarations -----------------------------------------------------

// --- API ---------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"

void app_main(void)
{
    printf("no noise controller\n");

    util_init();
    panel_init();

    panel_test_pattern(GPIO_NO_1, GPIO_NO_2);
}

#pragma GCC diagnostic pop

// --- Helpers -----------------------------------------------------------------
