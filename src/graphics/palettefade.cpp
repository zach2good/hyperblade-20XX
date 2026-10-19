#include "common/compat.h"
#include "graphics/bitmapformat.h"
#include <SDL3/SDL.h>
#include "colorpalette.h"
#include "globals.h"
#include "palettefade.h"
#include "visualeffectfade.h"
#include "palettescalarfade.h"
#include <cstring>

// kPaletteReserved on the first entry means the entries already animate, and the fade goes inactive.
// FUNCTION: HYPERX 0x0042cd10
PaletteFade::PaletteFade(const PaletteRange* pRange)
: m_nKind(0)
{
    m_iStart = pRange->nFirst;

    unsigned int nCount = pRange->nCount;

    m_cEntries = nCount;

    m_pEntries = std::make_unique<PaletteEntry[]>(nCount);

    memcpy(m_pEntries.get(), &g_pColorPalette->m_current[m_iStart], m_cEntries * sizeof(PaletteEntry));

    if (m_pEntries[0].peFlags == kPaletteReserved)
    {
        m_bActive = 0;

        return;
    }

    m_bActive = 1;

    for (unsigned int nEntry = 0; nEntry < m_cEntries; nEntry++)
    {
        m_pEntries[nEntry].peFlags = kPaletteReserved;
    }

    g_pColorPalette->SetEntries(m_iStart,
                                m_cEntries,
                                m_pEntries.get());

    m_iWorkStart = pRange->nFirst;

    nCount = pRange->nCount;

    m_cWorkEntries = nCount;

    m_pWorkEntries = std::make_unique<PaletteEntry[]>(nCount);

    memcpy(m_pWorkEntries.get(), &g_pColorPalette->m_current[m_iWorkStart], m_cWorkEntries * sizeof(PaletteEntry));

    m_duration = std::chrono::seconds{ 1 };

    m_stepped = Clock::Now();

    m_bPhase = 1;
}

// FUNCTION: HYPERX 0x0042ce60
PaletteFade::~PaletteFade()
{
    RestoreEntries();
}

// FUNCTION: HYPERX 0x0042ce90
void PaletteFade::RestoreEntries()
{
    if (m_bActive == 0)
    {
        return;
    }

    g_pColorPalette->AnimateEntries(m_iStart, m_cEntries, m_pEntries.get());

    for (unsigned int i = 0; i < m_cEntries; i++)
    {
        m_pEntries[i].peFlags = kPaletteNoCollapse;
    }

    g_pColorPalette->SetEntries(m_iStart, m_cEntries, m_pEntries.get());

    m_bActive = 0;
}
