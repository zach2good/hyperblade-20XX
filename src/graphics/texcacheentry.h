#ifndef HYPERBLADE_GRAPHICS_TEXCACHEENTRY_H
#define HYPERBLADE_GRAPHICS_TEXCACHEENTRY_H

#include "common/compat.h"
#include "common/staticassert.h"

class CD3DTexture;
class SoftTex;

// GLOBAL: HYPERX 0x004b4104
extern const char* g_pszBitmapExtension;

// Twenty four bytes.
class TexCacheEntry
{
public:
    TexCacheEntry(const char* pszName, HPALETTE hPalette);

    // Always answers null, and nothing reads it.
    void* DestroyTexObj();

    ~TexCacheEntry();

    // The constructor zeroes it, and nothing in the image reads it.
    int m_nUnused; // 0x000

    // m_bIsD3D says which.
    union
    {
        CD3DTexture* m_pD3DTexture; // 0x004
        SoftTex*     m_pSoftTexture;
    };

    char*          m_pszName; // 0x008
    int            m_bIsD3D;  // 0x00c
    short          m_wRefs;   // 0x010
    short          m_wUnused; // 0x012
    TexCacheEntry* m_pNext;   // 0x014
};

// The flag sits in the four bytes in front of the table.
// GLOBAL: HYPERX 0x004d2774
extern int g_bSoftTexPaletteBuilt;

STATIC_ASSERT(sizeof(TexCacheEntry) == 0x18);

#endif
