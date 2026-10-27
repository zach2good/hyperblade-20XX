// Globals only the sound files read; src/globals.cpp defines them in .data order.

#ifndef HYPERBLADE_SOUNDGLOBALS_H
#define HYPERBLADE_SOUNDGLOBALS_H
#include "common/compat.h"

#include "globals.h"

// GLOBAL: HYPERX 0x004d21dc
extern volatile int g_nSoundTimerDepth; // Shared with a re-entered timer tick.

#endif
