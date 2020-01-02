// shared.cpp
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
#include "shared.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Types -------------------------------------------------------------------

// --- Constants and macros ----------------------------------------------------

// --- Globals -----------------------------------------------------------------

// --- Helper declarations -----------------------------------------------------

// --- API ---------------------------------------------------------------------

void hex_dump(const void *data, size_t len)
{
    const uint8_t *data_8 = (const uint8_t *)data;
    size_t off, i;

    for (off = 0; off < len; off += 16) {
        Serial.printf("%05x ", off);

        for (i = 0; i < 16 && off + i < len; ++i) {
            Serial.printf(" %02x", data_8[off + i]);
        }

        while (i++ < 16) {
            Serial.print("   ");
        }

        Serial.print("  ");

        for (i = 0; i < 16 && off + i < len; ++i) {
            uint8_t ch = data_8[off + i];
            Serial.printf("%c", ch >= 32 && ch <= 126 ? ch : '.');
        }

        Serial.println();
    }
}

// --- Helpers -----------------------------------------------------------------

