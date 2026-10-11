#include "common/compat.h"
#include "graphics/ddrawcaps.h"
#include "damagebar.h"
#include "scrollbitmap.h"
#include "cddbitmap.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "math/vector.h"
#include "texture.h"
#include <ddraw.h>
#include <new.h>
#include <stdio.h>
#include "framedbarbitmap.h"
#include "common/pooledclasses.h"
#include "math/point.h"
#include "graphics/ddrawapi.h"

// FUNCTION: HYPERX 0x004390d0
FramedBarBitmap::FramedBarBitmap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
: CDDBitMap(pDDraw, pPrimary, pBack)
{
    ClearState();
}

// FUNCTION: HYPERX 0x00439140
void FramedBarBitmap::SetScrollColorMode(int nScrollColor)
{
    const int nNext = m_ptSteps.y + 1;

    m_dwScrollColor = nScrollColor;

    m_nScrollPos = nNext;
}

// FUNCTION: HYPERX 0x00439180
void FramedBarBitmap::ClearState()
{
    m_ptInset.y   = 2;
    m_ptInset.x   = 2;
    m_bHorizontal = 1;

    m_ptSteps.x = 0;

    m_flStep = (float)(m_ptSize.x - 4) * g_flOneNinetyNinth;

    m_ptSteps.y   = 99;
    m_nScrollPos  = 0;
    m_nUnusedB    = 1;
    m_nScrollEdge = 2;

    SetRect(&m_rcScroll, 0, 0, 0, 0);
}

// FUNCTION: HYPERX 0x00439200
FramedBarBitmap::~FramedBarBitmap()
{
    if (m_pBar != 0)
    {
        delete m_pBar;

        m_pBar = 0;
    }
}

// Loads "fr" plus the name, and the owned bitmap as the name plus a two-digit number.

#define BITMAP_KEY_COLOUR 0xffffff
#define BITMAP_NAME_MAX   0x100

// FUNCTION: HYPERX 0x00439270
int FramedBarBitmap::LoadFromFile(const char* pszName, int nUnused, int bColourKey, int)
{
    char      szName[BITMAP_NAME_MAX];
    DDrawCaps caps;

    m_nUnusedC = nUnused;
    m_pBar     = 0;

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

    // STRING: HYPERX 0x004be6a4
    sprintf(szName, "fr%s", pszName);

    const int nResult = CDDBitMap::LoadFromFile(szName, bColourKey, m_bSystemMemory);

    m_pBar = new CDDBitMap(this);

    if (m_pBar != 0)
    {
        // STRING: HYPERX 0x004bc1a4
        sprintf(szName, "%s%02d", pszName, 1);

        m_pBar->LoadFromFile(szName, bColourKey, m_bSystemMemory);
    }

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

// FUNCTION: HYPERX 0x00439400
int FramedBarBitmap::RestoreContents()
{
    if (m_pBar != 0)
    {
        m_pBar->RestoreContents();
    }

    return CDDBitMap::RestoreContents();
}

// FUNCTION: HYPERX 0x00439420
int FramedBarBitmap::DrawWithWidth(int nWidth)
{
    if (m_bVisible != 0)
    {
        if (m_pBar != 0)
        {
            Point size = m_pBar->GetSize();

            if (nWidth > size.x)
            {
                nWidth = size.x;
            }

            m_pBar->m_pClipRect->right = nWidth;

            m_pBar->Draw();
        }

        CDDBitMap::Draw();

        return m_bVisible;
    }

    return 0;
}

// The four zeros are the whole message; the owned bitmap reads the position back itself.
// FUNCTION: HYPERX 0x00439490
void FramedBarBitmap::SetPosition(Point pt)
{
    m_ptPosition.x = pt.x;
    m_ptPosition.y = pt.y;

    if (m_pBar != 0)
    {
        m_pBar->DispatchRect(0, 0, 0, 0);
    }
}
