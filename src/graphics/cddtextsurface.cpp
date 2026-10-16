#include "common/compat.h"
#include "texture.h"
#include "cddtext.h"
#include "globals.h"
#include "graphics/texture.h"
#include "common/pathstring.h"
#include <windows.h>
#include <stddef.h>
#include <ddraw.h>
#include "cddtextsurface.h"
#include "app/gameapp.h"
#include "math/point.h"
#include "graphics/ddrawapi.h"

// FUNCTION: HYPERX 0x004377f0
CDDTextSurface::CDDTextSurface()
{
    InitFields();
}

// FUNCTION: HYPERX 0x00437850
int CDDTextSurface::Draw()
{
    return Texture::Draw();
}

// FUNCTION: HYPERX 0x00437880
CDDTextSurface::CDDTextSurface(Texture* pSource)
: Texture(pSource)
{
    InitFields();
}

// FUNCTION: HYPERX 0x004378f0
CDDTextSurface::CDDTextSurface(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
: Texture(pDDraw, pPrimary, pBack)
{
    InitFields();
}

// FUNCTION: HYPERX 0x00437960
void CDDTextSurface::InitFields()
{
    // The client holds the name in EDI (0x0043796b).
    String* const pText = &m_strText;

    pText->Assign(String::SafeStrlen(""),
                  "");

    m_hFont      = 0;
    m_nTextMode  = OPAQUE;
    m_nTextAlign = 0;
}

// FUNCTION: HYPERX 0x004379b0
CDDTextSurface::~CDDTextSurface()
{
    if (m_hFont != 0)
    {
        DeleteObject(m_hFont);
        m_hFont = 0;
    }
}

// Eighteen points above 320 pixels wide, twelve at or below.

#define WIDE_SCREEN_WIDTH 0x140

#define FONT_POINTS_WIDE   18
#define FONT_POINTS_NARROW 12

// FUNCTION: HYPERX 0x00437a20
int CDDTextSurface::SetFontScaled(const char* pszName, const char* pszText, int bColourKey, int bSystemMemory)
{
    return SetFont(pszName, g_nPrevWidth <= WIDE_SCREEN_WIDTH ? FONT_POINTS_NARROW : FONT_POINTS_WIDE, pszText, bColourKey, bSystemMemory);
}

static_assert(offsetof(DDBLTFX, dwFillColor) == 0x50);

#define TEXT_PADDING 4

#define COLOUR_ENABLED  0xffffff
#define COLOUR_DISABLED 0x808080

// FUNCTION: HYPERX 0x00437a60
int CDDTextSurface::SetFont(const char* pszName, int nSize, const char* pszText, int bColourKey, int bSystemMemory)
{
    String* const pText = &m_strText;
    unsigned int  nLength;

    if (pszText == 0)
    {
        nLength = 0;
    }
    else
    {
        nLength = lstrlenA(pszText);
    }

    pText->Assign(nLength, pszText);

    if (m_hFont != 0)
    {
        DeleteObject(m_hFont);
    }

    m_hFont = CreateFontA(nSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, NONANTIALIASED_QUALITY, VARIABLE_PITCH, pszName);

    if (m_hFont == 0)
    {
        return 0;
    }

    const HDC hdc = GetDC(0);

    HGDIOBJ hOld = SelectObject(hdc, m_hFont);

    GetTextExtentPoint32A(hdc, pszText, strlen(pszText), (LPSIZE)&m_ptSize);

    m_ptSize.x += TEXT_PADDING;

    SelectObject(hdc, hOld);

    ReleaseDC(0, hdc);

    const int nResult = Texture::AllocSurface(m_ptSize, bColourKey, bSystemMemory);

    if (m_pSurface != 0)
    {
        m_ptSize = RenderText(bColourKey);

        RECT* const pRect = new RECT;

        m_pDestRect = pRect;

        SetRect(pRect, 0, 0, m_ptSize.x, m_ptSize.y);

        if (bColourKey != 0)
        {
            m_bltfx.dwFillColor = DDraw::ColourToPixel(m_pSurface, COLOUR_ENABLED);
        }
        else
        {
            m_bltfx.dwFillColor = DDraw::ColourToPixel(m_pSurface, COLOUR_DISABLED);
        }

        if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) != 0)
        {
            m_hr = m_pSurface->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
        }
        else
        {
            m_hr = 0;
        }

        if (m_hr != 0)
        {
            return 0;
        }
    }

    return nResult;
}

// The clip rectangle is ten thousand square because clipping is the surface's job.

#define CLIP_EXTENT 0x2710

// FUNCTION: HYPERX 0x00437c10
SurfacePoint CDDTextSurface::RenderText(int bColourKey)
{
    HDC   hdc;
    Point size;
    RECT  rect;

    size.Set(0, 0);

    if (bColourKey != 0)
    {
        m_nTextMode   = OPAQUE;
        m_dwFillColor = COLOUR_ENABLED;
    }
    else
    {
        m_dwFillColor = COLOUR_DISABLED;
    }

    SetColorMode(0xffff);

    if (m_pSurface != 0 && m_pSurface->GetDC(&hdc) == 0)
    {
        HGDIOBJ hOld = SelectObject(hdc, m_hFont);

        SetTextColor(hdc, m_dwScrollColor);
        SetBkColor(hdc, m_dwFillColor);
        SetBkMode(hdc, m_nTextMode);

        SetRect(&rect, 0, 0, CLIP_EXTENT, CLIP_EXTENT);

        ExtTextOutA(hdc, 0, 0, m_nTextMode, &rect, m_strText.m_pchData, m_strText.GetHeader()->nDataLength, 0);

        GetTextExtentPoint32A(hdc, m_strText.m_pchData, m_strText.GetHeader()->nDataLength, (SIZE*)&size);

        size.x += TEXT_PADDING;

        SelectObject(hdc, hOld);

        m_pSurface->ReleaseDC(hdc);
    }

    return SurfacePoint(size);
}

// FUNCTION: HYPERX 0x00437d60
int CDDTextSurface::OnSurfaceLost()
{
    return Texture::OnSurfaceLost();
}

// All three fatal strings name it CDDText::RestoreContents.
// FUNCTION: HYPERX 0x00437d70
int CDDTextSurface::RestoreContents()
{
    HDC hdc;

    if (m_pSurface == 0)
    {
        return 0;
    }

    m_hr = m_pSurface->GetDC(&hdc);

    if (m_hr == DDERR_SURFACELOST)
    {
        m_hr = m_pSurface->Restore();

        if (m_hr != 0)
        {
            // STRING: HYPERX 0x004be59c
            g_gameApp.Fatal("CDDText::RestoreContents Buffer Restore failed %d!",
                            m_hr & DDRAW_ERROR_CODE_MASK);
        }

        m_hr = m_pSurface->GetDC(&hdc);

        while (m_hr == DDERR_WASSTILLDRAWING)
        {
            m_hr = m_pSurface->GetDC(&hdc);
        }

        if (m_hr != 0)
        {
            // STRING: HYPERX 0x004be55c
            g_gameApp.Fatal("CDDText::RestoreContents Buffer getdc after restore failed %d!",
                            m_hr & DDRAW_ERROR_CODE_MASK);
        }
    }

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004be528
        g_gameApp.Fatal("CDDText::RestoreContents Buffer getdc 1 failed %d!",
                        m_hr & DDRAW_ERROR_CODE_MASK);
    }

    HGDIOBJ hOldFont = SelectObject(hdc, m_hFont);
    SetTextColor(hdc, m_dwScrollColor);
    SetBkColor(hdc, m_dwFillColor);
    SetTextAlign(hdc, m_nTextAlign);

    ExtTextOutA(hdc, 0, 0, m_nTextMode, m_pDestRect, m_strText.m_pchData, m_strText.GetHeader()->nDataLength, 0);

    SelectObject(hdc, hOldFont);

    m_pSurface->ReleaseDC(hdc);
    return 1;
}

// FUNCTION: HYPERX 0x00437ee0
int CDDTextSurface::DrawText(const char* pszText)
{
    if (m_bVisible != 0 && m_pSurface != 0)
    {
        if (pszText != 0)
        {
            m_strText.Assign(lstrlenA(pszText), pszText);

            RestoreContents();
        }

        return Texture::Draw();
    }

    return 0;
}
