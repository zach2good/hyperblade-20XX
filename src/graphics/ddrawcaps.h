#ifndef HYPERBLADE_DDRAWCAPS_H
#define HYPERBLADE_DDRAWCAPS_H

#include "common/staticassert.h"
#include <ddraw.h>

// The DirectX 6 DDCAPS is 0x17c bytes where this SDK's is 0x16c: DX6 appends a DDSCAPS2.
struct DDrawCaps : DDCAPS
{
    DWORD dwDX6SurfaceCaps;  // 0x16c
    DWORD dwDX6SurfaceCaps2; // 0x170
    DWORD dwDX6SurfaceCaps3; // 0x174
    DWORD dwDX6SurfaceCaps4; // 0x178
};

STATIC_ASSERT(sizeof(DDrawCaps) == 0x17c);

#endif
