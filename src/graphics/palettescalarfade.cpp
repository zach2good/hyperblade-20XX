#include "common/compat.h"
#include <algorithm>
#include "graphics/bitmapformat.h"
#include "palettefade.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "colorpalette.h"
#include "palettescalarfade.h"

// The duration is five hundred milliseconds.

constexpr auto kScalarKind     = 3;
constexpr auto kScalarDuration = 500;

// FUNCTION: HYPERX 0x0042cfc0
PaletteScalarFade::PaletteScalarFade(const PaletteRange* pRange)
: PaletteFade(pRange)
{
    unsigned int  cLeft  = m_cEntries;
    PaletteEntry* pEntry = m_pEntries;

    m_nKind = kScalarKind;

    m_cBrightest = 0;

    while (cLeft-- != 0)
    {
        unsigned char cGreen = pEntry->peGreen;

        if (cGreen <= m_cBrightest)
        {
            cGreen = m_cBrightest;
        }

        pEntry++;

        m_cBrightest = cGreen;
    }

    m_nDuration = kScalarDuration;
}

// The double constant at 0x004b1ac0.
#define MS_TO_SECONDS 0.001

// FUNCTION: HYPERX 0x0042d040
int PaletteScalarFade::Start(const PaletteEntry* pStartColour)
{
    if (m_bActive != 0)
    {
        g_pColorPalette->BuildFadeTable(m_pWorkEntries, static_cast<short>(m_cWorkEntries), *pStartColour, 0.1f);

        return 1;
    }

    return 0;
}

// The step is retimed on each turn so the fade keeps its wall-clock duration.
// FUNCTION: HYPERX 0x0042d080
int PaletteScalarFade::Tick()
{
    unsigned char cStep;

    const int bPhase = ApplyPhase(m_bPhase);

    if (bPhase != m_bPhase)
    {
        const unsigned char cRange   = m_cBrightest;
        const double        dSeconds = static_cast<double>(m_nDuration) * MS_TO_SECONDS;
        cStep                        = static_cast<unsigned char>(m_cBrightest / static_cast<float>(dSeconds * g_pGameEngine->m_clock.GetInvDelta()));

        if (cStep > cRange)
        {
            cStep = cRange;
        }

        m_cStep  = cStep;
        m_bPhase = bPhase;
    }

    return bPhase;
}

// Every entry arrived.
constexpr auto kFadeAllArrived = 0xff;

extern int g_nPaletteAnimateResult;

// Fades green out to nought or in to the source; 0xff in the mask means all eight arrived.
// FUNCTION: HYPERX 0x0042d100
int PaletteScalarFade::ApplyPhase(int bPhase)
{
    int nAnswer;

    unsigned int        nAt      = 0;
    PaletteEntry* const pEntries = m_pWorkEntries;

    if (bPhase != 0)
    {
        int nMask;
        int nValue;

        nMask = 0;

        for (nAt = 0; nAt < m_cWorkEntries; nAt++)
        {
            nValue = pEntries[nAt].peGreen;
            nValue -= m_cStep;

            nValue = (std::max)(0, nValue);

            pEntries[nAt].peGreen = static_cast<unsigned char>(nValue);

            if (static_cast<unsigned char>(nValue) == 0)
            {
                nMask += 1 << nAt;
            }
        }

        nAnswer = (nMask != kFadeAllArrived);
    }
    else
    {
        int           nMask;
        int           nValue;
        int           nLimit;
        PaletteEntry* pSource;

        nMask   = 0;
        pSource = m_pEntries;

        for (; nAt < m_cWorkEntries; nAt++)
        {
            nValue = pEntries[nAt].peGreen + m_cStep;

            nLimit = pSource[nAt].peGreen;

            if (nValue > nLimit)
            {
                nValue = nLimit;
            }

            pEntries[nAt].peGreen = static_cast<unsigned char>(nValue);

            if (pEntries[nAt].peGreen == pSource[nAt].peGreen)
            {
                nMask += 1 << nAt;
            }
        }

        nAnswer = (nMask == kFadeAllArrived);
    }

    g_nPaletteAnimateResult =
        g_pColorPalette->AnimateEntries(m_iWorkStart,
                                        m_cWorkEntries,
                                        m_pWorkEntries) &
        0xff;

    return nAnswer;
}
