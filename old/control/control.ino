// control.ino
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

#include <HardwareSerial.h>

#include "warnings.h"
#include "control.h"
#include "network.h"
#include "panels.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Types -------------------------------------------------------------------

// --- Constants and macros ----------------------------------------------------

// --- Globals -----------------------------------------------------------------

int32_t g_n_frames;
int32_t g_n_rows;
int32_t g_n_columns;

pixel_t g_frames[MAX_FRAMES][MAX_ROWS][MAX_COLUMNS];

// --- Helper declarations -----------------------------------------------------

// --- Framework ---------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"

void setup()
{
    Serial.begin(115200);

    panels_initialize();
    network_initialize();
}

void loop()
{
    int32_t frame_id = network_handle_io();

    if (frame_id >= 0) {
        panels_render_frame(frame_id);
    }
}

#pragma GCC diagnostic pop

// --- API ---------------------------------------------------------------------

// --- Helpers -----------------------------------------------------------------

