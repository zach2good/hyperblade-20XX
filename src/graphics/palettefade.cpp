#include "common/compat.h"
#include "graphics/bitmapformat.h"
#include <SDL3/SDL.h>
#include "colorpalette.h"
#include "globals.h"
#include "palettefade.h"
#include "visualeffectfade.h"
#include "palettescalarfade.h"
#include <cstring>

// PALETTE_RESERVED on the first entry means the entries already animate, and the fade goes inactive.
// FUNCTION: HYPERX 0x0042cd10
PaletteFade::PaletteFade(const PaletteRange* pRange)
: m_nKind(0)
{
    m_pEntries     = nullptr;
    m_pWorkEntries = nullptr;

    m_iStart = pRange->nFirst;

    unsigned int nCount = pRange->nCount;

    m_cEntries = nCount;

    auto* pEntries = new PaletteEntry[nCount];

    m_pEntries = pEntries;

    memcpy(pEntries, &g_pColorPalette->m_current[m_iStart], m_cEntries * sizeof(PaletteEntry));

    if (m_pEntries[0].peFlags == PALETTE_RESERVED)
    {
        m_bActive = 0;

        return;
    }

    m_bActive = 1;

    for (unsigned int nEntry = 0; nEntry < m_cEntries; nEntry++)
    {
        m_pEntries[nEntry].peFlags = PALETTE_RESERVED;
    }

    g_pColorPalette->SetEntries(m_iStart,
                                m_cEntries,
                                m_pEntries);

    m_iWorkStart = pRange->nFirst;

    nCount = pRange->nCount;

    m_cWorkEntries = nCount;

    pEntries = new PaletteEntry[nCount];

    m_pWorkEntries = pEntries;

    memcpy(pEntries, &g_pColorPalette->m_current[m_iWorkStart], m_cWorkEntries * sizeof(PaletteEntry));

    m_nDuration = 1000;

    m_dwStepped = (unsigned int)SDL_GetTicks();

    m_bPhase = 1;
}

// The two blocks are freed without a null check.
// FUNCTION: HYPERX 0x0042ce60
PaletteFade::~PaletteFade()
{
    RestoreEntries();

    delete[] m_pEntries;
    delete[] m_pWorkEntries;
}

// FUNCTION: HYPERX 0x0042ce90
void PaletteFade::RestoreEntries()
{
    if (m_bActive == 0)
    {
        return;
    }

    g_pColorPalette->AnimateEntries(m_iStart, m_cEntries, m_pEntries);

    for (unsigned int i = 0; i < m_cEntries; i++)
    {
        m_pEntries[i].peFlags = PALETTE_NOCOLLAPSE;
    }

    g_pColorPalette->SetEntries(m_iStart, m_cEntries, m_pEntries);

    m_bActive = 0;
}
