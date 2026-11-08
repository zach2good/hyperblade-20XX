#include "common/compat.h"
#include "graphics/passbar.h"
#ifdef HYPERX_V100
#include "app/gameapp.h"
#include "graphics/ddrawapi.h"
#endif

// FUNCTION: HYPERX100 0x00443e70
// FUNCTION: HYPERX 0x00438f40
int PassBar::DrawAtPos(int nPosition)
{
    if (m_bVisible != 0)
    {
        CDDBitMap::Draw();

        return UpdateScrollPos(nPosition);
    }

    return 0;
}

// FUNCTION: HYPERX100 0x00443ea0
// FUNCTION: HYPERX 0x00438f70
int PassBar::AutoScroll()
{
    if (m_bVisible != 0)
    {
        CDDBitMap::Draw();

        return UpdateScrollPos(m_nAutoPosition);
    }

    return 0;
}

// FUNCTION: HYPERX100 0x00443ed0
// FUNCTION: HYPERX 0x00438fa0
int PassBar::UpdateScrollPos(int nPosition)
{
    int nChanged = 0;
    int nOther;

    m_nPosition = nPosition;

    if (nPosition > m_ptRange.y)
    {
        m_nPosition = m_ptRange.y;
        nChanged    = 1;
    }

    const int nLower = m_ptRange.x;

    if (m_nPosition < nLower)
    {
        m_nPosition = nLower;
        nChanged    = 1;
    }

    if (m_bHorizontal != 0)
    {
        nOther = m_ptInset.x;

        m_ptMarker.x = (int)((float)(nLower + m_nPosition) * m_flScale) - m_ptMarkerOffset.x + m_ptPosition.x + nOther;

        if (m_ptMarker.x > nOther)
        {
            m_pMarkerTexture->SetPosition(m_ptMarker);

            m_pMarkerTexture->Draw();
        }
    }
    else
    {
        nOther = m_ptInset.y;

        m_ptMarker.y = m_ptPosition.y - (int)((float)(nLower + m_nPosition) * m_flScale) - m_ptMarkerOffset.y - nOther + m_ptSize.y;

        if (m_ptMarker.y < m_ptSize.y - nOther)
        {
            m_pMarkerTexture->SetPosition(m_ptMarker);

            m_pMarkerTexture->Draw();
        }
    }

    return nChanged;
}

#ifdef HYPERX_V100
// FUNCTION: HYPERX100 0x00443830
PassBar::PassBar(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
: CDDBitMap(pDDraw, pPrimary, pBack)
{
    m_ptRange.Zero();
    m_ptMarkerOffset.Zero();
    m_ptMarker.Zero();

    ClearState();
}

// FUNCTION: HYPERX100 0x00443920
void PassBar::SetScrollColorMode(int nScrollColor)
{
    m_dwScrollColor = nScrollColor;
    m_nPosition     = m_ptRange.y + 1;
}

// FUNCTION: HYPERX100 0x00443940
PassBar::PassBar()
{
    m_ptRange.Zero();
    m_ptMarkerOffset.Zero();
    m_ptMarker.Zero();

    ClearState();
}

// FUNCTION: HYPERX100 0x004439f0
void PassBar::ClearState()
{
    m_ptInset.y      = 2;
    m_ptInset.x      = 2;
    m_pMarkerTexture = 0;
    m_bHorizontal    = 1;
    m_ptRange.x      = 0;
    m_nPosition      = 0;
    m_nUnusedB       = 1;
    m_ptMarker.y     = 0;
    m_ptRange.y      = 100;
    m_ptMarker.x     = 0;
    m_flScale        = (float)(m_ptSize.x - 4) * 0.01f;
}

// FUNCTION: HYPERX100 0x00443a80
PassBar::~PassBar()
{
    if (m_pMarkerTexture != 0)
    {
        delete m_pMarkerTexture;
    }

    m_pMarkerTexture = 0;
}

// FUNCTION: HYPERX100 0x00443b10
int PassBar::LoadFromFile(const char* pszBarName, const char* pszMarkerName, int bColourKey, int bSystemMemory)
{
    int nResult = CDDBitMap::LoadFromFile(pszBarName, bColourKey, bSystemMemory);

    if (m_pSurface != 0)
    {
        if (bColourKey != 0)
        {
            m_dwFillColor = DDraw::ColourToPixel(m_pSurface, 0xffffff);
        }

        if (m_ptSize.x < m_ptSize.y)
        {
            m_bHorizontal = 0;
        }

        SetClipRect(1, m_ptInset);
    }

    m_pMarkerTexture = new CDDBitMap(this);

    if (m_pMarkerTexture == 0)
    {
        // STRING: HYPERX100 0x00510ff4
        g_gameApp.Error("Cannot create surface for Meter indicator!");
    }

    nResult += ((CDDBitMap*)m_pMarkerTexture)->LoadFromFile(pszMarkerName, 1, m_bSystemMemory);

    m_ptMarkerOffset = m_pMarkerTexture->GetSize();
    m_ptMarkerOffset.x >>= 1;
    m_ptMarkerOffset.y >>= 1;

    return nResult;
}

// FUNCTION: HYPERX100 0x00443cb0
void PassBar::SetClipRect(int nClipped, Point pt)
{
    CDDBitMap::SetClipRect(nClipped, pt);

    const Point* const pRange = &m_ptRange;
    const int          nHigh  = pRange->y;

    m_ptRange.x = pRange->x;
    m_ptRange.y = nHigh;

    if (m_bHorizontal != 0)
    {
        m_flScale = (float)(m_ptSize.x - m_ptInset.x * 2) / (float)m_ptRange.y;
    }
    else
    {
        m_flScale = (float)(m_ptSize.y - m_ptInset.y * 2) / (float)m_ptRange.y;
    }

    if (m_bClipped != 0)
    {
        m_ptMarker.x = m_ptInset.x;
    }
}

// FUNCTION: HYPERX100 0x00443d80
int PassBar::RestoreContents()
{
    int bResult = CDDBitMap::RestoreContents();

    if (m_pMarkerTexture != 0)
    {
        bResult |= m_pMarkerTexture->OnSurfaceLost();
    }

    return bResult;
}

// The marker sits at the bar's horizontal centre and on its bottom edge.
// FUNCTION: HYPERX100 0x00443db0
void PassBar::SetPosition(Point pt)
{
    CDDBitMap::SetPosition(pt);

    m_ptMarker = pt;
    m_ptMarker.x += m_ptSize.x >> 1;

    const Point size = m_pMarkerTexture->GetSize();

    m_ptMarker.y += m_ptSize.y - size.y;

    m_pMarkerTexture->SetPosition(m_ptMarker);
}

// FUNCTION: HYPERX100 0x00443e50
int PassBar::Step()
{
    return DrawAtPos(m_nPosition + m_nUnusedB);
}
#endif
