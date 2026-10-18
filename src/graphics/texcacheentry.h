#ifndef HYPERBLADE_GRAPHICS_TEXCACHEENTRY_H
#define HYPERBLADE_GRAPHICS_TEXCACHEENTRY_H

#include "common/compat.h"
#include "graphics/softtex.h"
#include <memory>
#include <string>

// GLOBAL: HYPERX 0x004b4104
extern const char* g_pszBitmapExtension;

// Twenty four bytes.
class TexCacheEntry
{
public:
    explicit TexCacheEntry(const char* pszName);

    // Always answers null, and nothing reads it.
    void* DestroyTexObj();

    ~TexCacheEntry();
    TexCacheEntry(const TexCacheEntry&)            = delete;
    TexCacheEntry& operator=(const TexCacheEntry&) = delete;
    TexCacheEntry(TexCacheEntry&&)                 = delete;
    TexCacheEntry& operator=(TexCacheEntry&&)      = delete;

    // The constructor zeroes it, and nothing in the image reads it.
    int m_nUnused; // 0x000

    std::unique_ptr<SoftTex>       m_pSoftTexture; // 0x004
    std::string                    m_strName;      // 0x008
    short                          m_wRefs;        // 0x010
    short                          m_wUnused;      // 0x012
    std::unique_ptr<TexCacheEntry> m_pNext;        // 0x014
};

// The flag sits in the four bytes in front of the table.
// GLOBAL: HYPERX 0x004d2774
extern int g_bSoftTexPaletteBuilt;

#endif
