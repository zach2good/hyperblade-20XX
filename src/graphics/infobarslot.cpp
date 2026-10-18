#include "common/compat.h"
#include "rlespritesurface.h"
#include "engine/gameengine.h"
#include "ui/infobar.h"
#include "graphics/infobarradar.h"
#include "palettefade.h"
#include "globals.h"
#include "graphics/infobarslot.h"
#include "graphics/swrend.h"

// A frame of nought draws one block; otherwise each segment is a unit wider and a step taller.

// FUNCTION: HYPERX 0x0044f6d0
int InfoBarSlot::Draw()
{
    int nWidth;

    const int nRange = m_nFrameCount;

    if (nRange == 0)
    {
        return 0;
    }

    const int nFar = m_nMaxHeight;

    const int nY = m_ptDrawOffset.y + m_ptPosition.y;

    const float flStep = static_cast<float>(nFar - m_nMinHeight) / nRange;

    int nX = m_ptDrawOffset.x + m_ptPosition.x;

    if (m_nFrame == 0)
    {
        if (m_flScalar > 0.0f)
        {
            nWidth = static_cast<int>(m_nUnitWidth * m_flScalar);

            if (nWidth <= 0)
            {
                nWidth = 1;
            }

            SWRend::FillRect(nX, nY - m_nMinHeight + nFar, nWidth, m_nMinHeight, m_nColour);
        }
    }
    else
    {
        for (int nIndex = 0; nIndex < m_nFrame; nIndex++)
        {
            const int nPos = static_cast<int>(static_cast<float>(nIndex) * flStep) + m_nMinHeight;

            SWRend::FillRect(nX, nY - nPos + nFar, m_nUnitWidth, nPos, m_nColour);

            nX += m_nUnitWidth;
        }
    }

    return 1;
}

// 0x00451c60 is CDDText::CDDText in src/graphics/cddtext.h, which carries its marker.
