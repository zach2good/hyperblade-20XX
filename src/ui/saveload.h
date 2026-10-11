#ifndef HYPERBLADE_SAVELOAD_H
#define HYPERBLADE_SAVELOAD_H

#include "uiscreen.h"
#include "engine/gameengine.h"

struct SlotRow
{
    int nX;
    int nY;
};

// GLOBAL: HYPERX 0x004c4e70
extern int g_nSlotRowCount;
// GLOBAL: HYPERX 0x004c4e78
extern SlotRow g_aSlotRows[10];

#endif
