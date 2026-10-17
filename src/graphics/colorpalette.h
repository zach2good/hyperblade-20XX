// 0x474 bytes. The arena, object, team, Rok, open and general ranges feed the fade table.

#ifndef HYPERBLADE_COLORPALETTE_H
#define HYPERBLADE_COLORPALETTE_H
#include "common/compat.h"
#include <cstdint>

#include "common/pathstring.h"
#include "graphics/paletterange.h"
#include "graphics/cgpalette.h"

class ColorPalette
{
public:
    ColorPalette();
    // What shutdown runs before it frees the palette; the ranges need no teardown.
    void Release();

    unsigned int Load(void);
    unsigned int LoadCustomPalette(void);

    int LoadPaletteEntries(const char* pszName, char bRemap);

    void ReloadPalette(short nMode);

    int  SetEntries(unsigned int iStart, unsigned int cEntries, PALETTEENTRY* pEntries);
    void SyncDIBColorTable(unsigned int iStart, unsigned int cEntries);
    char AnimateEntries(unsigned int iStart, unsigned int cEntries, PALETTEENTRY* pEntries);
    void BuildFadeTable(PALETTEENTRY* pEntries, short nCount, PALETTEENTRY peColour, float flFloor);

    void Build();

    // The entry nearest a 0x00BBGGRR colour, skipping one index, or none for -1.
    int NearestIndex(std::uint32_t colour, int nSkip) const;

    // The colour-cycle range a fade kind occupies.
    PaletteRange GetFrameRange(int nKind);

    PALETTEENTRY* GetRemappedEntry(PALETTEENTRY* pOut, int nIndex);
    char          IsEntryUsable(int nIndex);

    unsigned short m_wVersion;     // 0x000
    unsigned short m_wNumEntries;  // 0x002
    PALETTEENTRY   m_entries[256]; // 0x004
    // What the screen shows: the entries, with the animated ones as the fades last left them.
    PALETTEENTRY m_current[256];
    String       m_name; // 0x40c
    // CUSTOM.PAL's rows in file order. The bounds hold a lower and an upper index, and the
    // ramp length the small and the large ramp's length less one.
    PaletteRange m_rampBounds;      // 0x410
    PaletteRange m_smallRampBounds; // 0x418
    PaletteRange m_largeRampBounds; // 0x420
    PaletteRange m_rampLength;      // 0x428
    PaletteRange m_arenaRange;      // 0x430
    PaletteRange m_objectRange;     // 0x438
    PaletteRange m_teamARange;      // 0x440
    PaletteRange m_teamBRange;      // 0x448
    PaletteRange m_rokRange;        // 0x450
    PaletteRange m_openRange;       // 0x458
    PaletteRange m_generalRange;    // 0x460
    // Team A's first and last index; the last sits where a range keeps its count.
    PaletteRange m_remapRange; // 0x468

    static unsigned char LookupColorIntensity(ColorPalette* pPalette,
                                              unsigned char cShade,
                                              float         flIntensityVal,
                                              unsigned char cColour);
};

#endif
