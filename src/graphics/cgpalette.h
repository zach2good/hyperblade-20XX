#ifndef HYPERBLADE_CGPALETTE_H
#define HYPERBLADE_CGPALETTE_H

#include "common/compat.h"
#include <stdio.h>

// A GIF's logical screen descriptor and global colour table.
struct GifScreen
{
    char  aSignature[3]; // 0x000
    char  aVersion[3];   // 0x003
    short nWidth;        // 0x006
    short nHeight;       // 0x008
    // Bit 7 flags a colour table of 2^(n+1) entries, n the low three bits.
    unsigned char cFlags;           // 0x00a
    unsigned char cBackground;      // 0x00b
    unsigned char cAspect;          // 0x00c
    unsigned char aColours[256][3]; // 0x00d, red, green and blue
};

static_assert(sizeof(GifScreen) == 0x30e);

namespace CGPalette
{

// Answers the number of colours in the table, or -1 on a read error.
int ReadGifScreen(GifScreen* pScreen, FILE* pFile);

} // namespace CGPalette

#endif
