#ifndef HYPERBLADE_TORCHBITMAPS_H
#define HYPERBLADE_TORCHBITMAPS_H

#include "ui/screens/teamsetupuiscreen.h"

#include "pausemenu.h"

struct TorchBitmaps
{
    const char* pszWon;
    const char* pszPending;
    const char* pszLost;
};

// GLOBAL: HYPERX 0x004c3f88
extern TorchBitmaps g_aTorchBitmaps[14];

#endif
