#include "common/compat.h"
#include <SDL3/SDL.h>
#include "visualeffectfade.h"
#include "globals.h"
#include "colorpalette.h"
#include "palettefade.h"

// The phase turns only after the duration has passed since the last advance.
// FUNCTION: HYPERX 0x0042cf20
int VisualEffectFade::Tick()
{
    if ((DWORD)SDL_GetTicks() - m_dwStepped > m_nDuration)
    {
        m_bPhase = ApplyPhase(m_bPhase);

        m_dwStepped = (DWORD)SDL_GetTicks();

        return 1;
    }

    ApplyPhase(m_bPhase);

    return 0;
}

// FUNCTION: HYPERX 0x0042cf70
int VisualEffectFade::ApplyPhase(int bPhase)
{
    if (bPhase != 0)
    {
        g_nPaletteAnimated = g_pColorPalette->AnimateEntries(m_iStart, m_cEntries, m_pEntries) & 0xff;
    }
    else
    {
        g_nPaletteAnimated = g_pColorPalette->AnimateEntries(m_iWorkStart, m_cWorkEntries, m_pWorkEntries) & 0xff;
    }

    return bPhase == 0;
}

// Folded with PaletteScalarFade::Start onto one address, so it carries no marker.
int VisualEffectFade::Start(const PALETTEENTRY* pStartColour)
{
    if (m_bActive != 0)
    {
        g_pColorPalette->BuildFadeTable(m_pWorkEntries, (short)m_cWorkEntries, *pStartColour, 0.1f);

        return 1;
    }

    return 0;
}
