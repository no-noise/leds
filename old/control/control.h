// control.h
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

#include "warnings.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Types -------------------------------------------------------------------

struct pixel_t {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

// --- Constants and macros ----------------------------------------------------

#define MAX_COLUMNS 10
#define MAX_ROWS 10
#define MAX_FRAMES 100

// --- Globals -----------------------------------------------------------------

extern int32_t g_n_frames;
extern int32_t g_n_rows;
extern int32_t g_n_columns;

extern pixel_t g_frames[MAX_FRAMES][MAX_ROWS][MAX_COLUMNS];

// --- API ---------------------------------------------------------------------

