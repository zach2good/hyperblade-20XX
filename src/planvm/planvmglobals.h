// Globals only the planvm files read; src/globals.cpp defines them in .data order.

#ifndef HYPERBLADE_PLANVMGLOBALS_H
#define HYPERBLADE_PLANVMGLOBALS_H
#include "common/compat.h"

#include "globals.h"

// When the announcer last picked a line.
// GLOBAL: HYPERX 0x004cfa54
extern int g_nAnnouncerLineMs;

// GLOBAL: HYPERX 0x004b4110
extern const char* const g_pszPbnExtension;

// The name both keyword tables end their last entry with.
// GLOBAL: HYPERX 0x004b3e4c
extern const char* const g_pszPlanVmEmptyName;

// Progress through a four-step sequence; an out-of-order step resets it.
// GLOBAL: HYPERX 0x004cfa5c
extern int g_nPlanSequenceStep;

// GLOBAL: HYPERX 0x004b3e30
extern const char* const g_pszDefaultPlanName;

// Where each of the two secondary camera cycles has got to.
// GLOBAL: HYPERX 0x004bb2d4
extern short g_nSecondaryCamCursorA;

// GLOBAL: HYPERX 0x004bb2d6
extern short g_nSecondaryCamCursorB;

// GLOBAL: HYPERX 0x004b1778
extern const float g_flFrameRateReference;

// The period timer's rate, a negative hundredth so the count runs the other way.
// GLOBAL: HYPERX 0x004b3740
extern const double g_dPeriodScale;

// The registered behaviours, walked until the first null; a scratch entry follows at 0x004cb5b8.
// GLOBAL: HYPERX 0x004cb54c
extern class PooledOwnerRoot* g_apBehaviorRegistry[27];

#endif
