#include "common/compat.h"
#include <SDL3/SDL.h>
#include "colorpalette.h"
#include "globals.h"
#include "palettefade.h"
#include "visualeffectfade.h"
#include "palettescalarfade.h"
#include <windows.h>

// PC_RESERVED on the first entry means the entries already animate, and the fade goes inactive.
// FUNCTION: HYPERX 0x0042cd10
PaletteFade::PaletteFade(const PaletteRange* pRange)
: m_nKind(0)
{
    m_pEntries     = 0;
    m_pWorkEntries = 0;

    m_iStart = pRange->nFirst;

    unsigned int nCount = pRange->nCount;

    m_cEntries = nCount;

    PALETTEENTRY* pEntries = new PALETTEENTRY[nCount];

    m_pEntries = pEntries;

    GetPaletteEntries(g_pColorPalette->m_hPalette, m_iStart, m_cEntries, pEntries);

    if (m_pEntries[0].peFlags == PC_RESERVED)
    {
        m_bActive = 0;

        return;
    }

    m_bActive = 1;

    for (unsigned int nEntry = 0; nEntry < m_cEntries; nEntry++)
    {
        m_pEntries[nEntry].peFlags = PC_RESERVED;
    }

    g_pColorPalette->SetEntries(m_iStart,
                                m_cEntries,
                                m_pEntries);

    ColorPalette* const pPalette = g_pColorPalette;

    HPALETTE hOld = SelectPalette(pPalette->m_hdc, pPalette->m_hPalette, 0);

    pPalette->m_hOldPalette = hOld;

    RealizePalette(pPalette->m_hdc);

    m_iWorkStart = pRange->nFirst;

    nCount = pRange->nCount;

    m_cWorkEntries = nCount;

    pEntries = new PALETTEENTRY[nCount];

    m_pWorkEntries = pEntries;

    GetPaletteEntries(g_pColorPalette->m_hPalette, m_iWorkStart, m_cWorkEntries, pEntries);

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
        m_pEntries[i].peFlags = PC_NOCOLLAPSE;
    }

    g_pColorPalette->SetEntries(m_iStart, m_cEntries, m_pEntries);

    ColorPalette* const pPalette = g_pColorPalette;

    pPalette->m_hOldPalette = SelectPalette(pPalette->m_hdc, pPalette->m_hPalette, 0);

    ::RealizePalette(pPalette->m_hdc);

    m_bActive = 0;
}
