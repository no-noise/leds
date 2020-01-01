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

