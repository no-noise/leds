// --- Includes ----------------------------------------------------------------

#include "warnings.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Types -------------------------------------------------------------------

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} pixel_t;

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

