#include "common/compat.h"
#include "graphics/ddrawcaps.h"
#include "cddbitmap.h"
#include "damagebar.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "math/vector.h"
#include "ui/hudglobals.h"
#include "texture.h"
#include "scrollbitmap.h"
#include <ddraw.h>
#include "graphics/ddrawapi.h"

// FUNCTION: HYPERX 0x00438d40
void SteppedScrollBitmap::SetDirty()
{
    m_bDirty = 1;
}

#define SCROLL_STEPS_WIDE   15
#define SCROLL_STEPS_NARROW 8
#define SCROLL_BAND_HEIGHT  2

// A wide display scrolls fifteen steps across and one down, a narrow one eight across and none.
// FUNCTION: HYPERX 0x00438d50
int SteppedScrollBitmap::InitAndLoad(const char* pszName, int nCount, int bColourKey, int bSystemMemory)
{
    if (g_hudSurfaces.bHud320 != 0)
    {
        m_nStepX     = SCROLL_STEPS_NARROW;
        m_nStepCount = nCount;
        m_nStepY     = 0;
    }
    else
    {
        m_nStepX     = SCROLL_STEPS_WIDE;
        m_nStepCount = nCount;
        m_nStepY     = 1;
    }

    m_nBandHeight = SCROLL_BAND_HEIGHT;

    return LoadFromFile(pszName, bColourKey, bSystemMemory);
}

// With m_bHorizontal clear the original reads the end unwritten.
// FUNCTION: HYPERX 0x00438dc0
int SteppedScrollBitmap::DrawScrolled()
{
    int  nEnd = 0;
    RECT rcScreen;

    if (m_bHorizontal != 0)
    {
        nEnd = m_nScrollEdge;
    }
    else
    {
        m_rcScroll.top = m_nScrollEdge;
    }

    m_bltfx.dwFillColor = m_dwScrollColor;

    if (m_pSurface == 0)
    {
        return 0;
    }

    RECT rc = m_rcScroll;
    rc.top  = rc.bottom - m_nBandHeight;

    int nRight = m_nStepX - m_ptInset.x;

    if (nRight > nEnd)
    {
        nRight = nEnd;
    }

    while (nRight <= nEnd)
    {
        rc.right = nRight;

        if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) != 0)
        {
            m_hr = m_pSurface->Blt(&rc, 0, &rc, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
        }
        else
        {
            SetRect(&rcScreen, m_ptPosition.x + rc.left, m_ptPosition.y + rc.top, m_ptPosition.x + rc.right, m_ptPosition.y + rc.bottom);

            m_hr = g_pSurfaceBack->Blt(&rcScreen, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
        }

        if (m_hr == DDERR_SURFACELOST)
        {
            RestoreContents();
        }

        rc.top -= m_nStepY;

        if (rc.top < m_rcScroll.top)
        {
            rc.top = m_rcScroll.top;
        }

        rc.left = rc.right;

        if (nRight == nEnd)
        {
            break;
        }

        nRight += m_nStepX;

        if (nRight > nEnd)
        {
            nRight = nEnd;
        }
    }

    return m_hr == 0;
}
