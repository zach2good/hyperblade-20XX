#include "common/compat.h"
#include "cddbitmap.h"
#include "damagebar.h"
#include "scrollbitmap.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "graphics/ddrawcaps.h"
#include "graphics/ddrawapi.h"

// FUNCTION: HYPERX 0x004386c0
ScrollBitmap::ScrollBitmap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
: CDDBitMap(pDDraw, pPrimary, pBack)
{
    ClearState();
}

// FUNCTION: HYPERX 0x00438730
void ScrollBitmap::SetScrollColorMode(int nScrollColor)
{
    m_dwScrollColor = nScrollColor;
}

// FUNCTION: HYPERX 0x00438760
ScrollBitmap::ScrollBitmap()
{
    ClearState();
}

// FUNCTION: HYPERX 0x004387c0
void ScrollBitmap::ClearState()
{
    m_ptInset.y   = 2;
    m_ptInset.x   = 2;
    m_nScrollEdge = 2;
    m_bHorizontal = 1;

    const float flStep = (float)(m_ptSize.x - 4) * 0.01f;

    m_flStep = flStep;

    m_ptSteps.x  = 0;
    m_ptSteps.y  = 100;
    m_nScrollPos = 0;
    m_nUnusedB   = 1;

    SetRect(&m_rcScroll, 0, 0, 0, 0);

    m_bDirty = 0;
}

// FUNCTION: HYPERX 0x00438850
ScrollBitmap::~ScrollBitmap()
{
}

#define BITMAP_KEY_COLOUR 0xffffff

// FUNCTION: HYPERX 0x00438860
int ScrollBitmap::LoadFromFile(const char* pszName, int bColourKey, int)
{
    DDrawCaps caps;

    if (bColourKey != 0)
    {
        caps.dwSize = sizeof(caps);

        m_hr = g_pSurfaceDDraw->GetCaps((LPDDCAPS)&caps, 0);

        if (m_hr != 0)
        {
            return 0;
        }

        if ((caps.dwCaps & DDCAPS_COLORKEYHWASSIST) != 0)
        {
            m_bSystemMemory = 1;
        }
        else
        {
            m_bSystemMemory = 0;
        }
    }

    const int nResult = CDDBitMap::LoadFromFile(pszName, bColourKey, m_bSystemMemory);

    if (m_pSurface != 0)
    {
        if (bColourKey != 0)
        {
            m_dwFillColor = DDraw::ColourToPixel(m_pSurface, BITMAP_KEY_COLOUR);
        }

        if (m_ptSize.x < m_ptSize.y)
        {
            m_bHorizontal = 0;
        }

        SetClipRect(1, m_ptInset);
    }

    return nResult;
}

// FUNCTION: HYPERX 0x00438a10
int ScrollBitmap::RestoreContents()
{
    if (CDDBitMap::RestoreContents() != 0)
    {
        return DrawScrolled();
    }

    return 0;
}

// The float input is the lower bound plus the position, and both edge comparisons are unsigned.
// FUNCTION: HYPERX 0x00438a30
int ScrollBitmap::ScrollTo(int nPosition)
{
    if (m_bVisible != 0)
    {
        int nChanged = 0;

        if (nPosition == m_nScrollPos && m_bDirty == 0)
        {
            if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) == 0)
            {
                DrawScrolled();
            }
        }
        else
        {
            if (nPosition < m_nScrollPos)
            {
                BltFillColour(m_dwFillColor);
            }

            m_nScrollPos = nPosition;

            if (nPosition > m_ptSteps.y)
            {
                m_nScrollPos = m_ptSteps.y;
                nChanged     = 1;
            }

            const int nLower = m_ptSteps.x;

            if (m_nScrollPos < nLower)
            {
                m_nScrollPos = nLower;
                nChanged     = 1;
            }

            if (m_bHorizontal != 0)
            {
                m_nScrollEdge = (int)((float)(nLower + m_nScrollPos) * m_flStep) + m_ptInset.x;

                if ((unsigned int)m_nScrollEdge > (unsigned int)m_ptInset.x)
                {
                    DrawScrolled();
                }
            }
            else
            {
                m_nScrollEdge = m_ptSize.y - (int)((float)(nLower + m_nScrollPos) * m_flStep) - m_ptInset.y;

                if ((unsigned int)m_nScrollEdge < (unsigned int)(m_ptSize.y - m_ptInset.y))
                {
                    DrawScrolled();
                }
            }
        }

        m_bDirty = 0;

        CDDBitMap::Draw();

        return nChanged;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00438c50
int ScrollBitmap::DrawScrolled()
{
    RECT rc;

    if (m_bHorizontal != 0)
    {
        m_rcScroll.right = m_nScrollEdge;
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

    if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) != 0)
    {
        m_hr = m_pSurface->Blt(&m_rcScroll, 0, &m_rcScroll, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
    }
    else
    {
        SetRect(&rc, m_ptPosition.x + m_rcScroll.left, m_ptPosition.y + m_rcScroll.top, m_ptPosition.x + m_rcScroll.right, m_ptPosition.y + m_rcScroll.bottom);

        m_hr = g_pSurfaceBack->Blt(&rc, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
    }

    if (m_hr == DDERR_SURFACELOST)
    {
        RestoreContents();
    }

    return m_hr == 0;
}
