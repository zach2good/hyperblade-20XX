#include "common/compat.h"
#include "graphics/ddrawcaps.h"
#include "texture.h"
#include "globals.h"
#include "graphics/texture.h"
#include "common/pathstring.h"
#include "graphicsglobals.h"
#include "math/vector.h"
#include "d3ddevice.h"
#include <ddraw.h>
#include <string.h>
#include "app/gameapp.h"
#include "math/point.h"
#include "graphics/ddrawapi.h"

// The original's messages call this class CDDSurface.

// FUNCTION: HYPERX 0x00433890
void Texture::SetVisible(int nVisible)
{
    m_bVisible = nVisible;
}

// FUNCTION: HYPERX 0x004338a0
void Texture::SetPosition(Point pt)
{
    m_ptPosition.x = pt.x;
    m_ptPosition.y = pt.y;
}

// FUNCTION: HYPERX 0x004338c0
Point Texture::GetPosition()
{
    return m_ptPosition;
}

// FUNCTION: HYPERX 0x004338e0
IDirectDrawSurface* Texture::GetSurface()
{
    return m_pSurface;
}

// FUNCTION: HYPERX 0x004338f0
Texture* Texture::GetSourceSurface()
{
    return m_pSourceSurface;
}

// FUNCTION: HYPERX 0x00433900
Point Texture::GetSize()
{
    return m_ptSize;
}

// FUNCTION: HYPERX 0x00433920
int Texture::GetForeColor()
{
    return m_dwScrollColor;
}

// FUNCTION: HYPERX 0x00433930
int Texture::GetBgColor()
{
    return m_dwFillColor;
}

// FUNCTION: HYPERX 0x00433940
Point Texture::GetInset()
{
    return m_ptInset;
}

// FUNCTION: HYPERX 0x00433960
void Texture::Unlock()
{
    m_pSurface->Unlock(0);
}

SurfacePoint g_ptSurfacePixels;

// FUNCTION: HYPERX 0x00436db0
Texture::Texture()
: m_nUnusedA(0)
{
    Init(0);
}

// FUNCTION: HYPERX 0x00436e10
Texture::Texture(Texture* pSource)
: m_nUnusedA(0)
{
    Init(pSource);
}

// The pixel counters are cleared only when nothing has been recorded before.
// FUNCTION: HYPERX 0x00436e50
Texture::Texture(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
: m_nUnusedA(0)
{
    if (g_pSurfaceDDraw == 0)
    {
        g_ptSurfacePixels.y = 0;
        g_ptSurfacePixels.x = 0;
    }

    g_pSurfaceDDraw = pDDraw;
    g_pSurfaceFront = pPrimary;
    g_pSurfaceBack  = pBack;

    Init(0);
}

// Fatal when a DirectDraw pointer is missing. The GetCaps result is overwritten with nought.

// FUNCTION: HYPERX 0x00436ed0
void Texture::Init(Texture* pSource)
{
    DDrawCaps caps;

    m_pSourceSurface = pSource;
    m_pSurface       = 0;

    memset(&m_bltfx, 0, sizeof(m_bltfx));

    m_bltfx.dwSize = sizeof(m_bltfx);

    m_ptSize.y = 0;
    m_ptSize.x = 0;

    if (m_pSourceSurface != 0)
    {
        m_ptPosition.Point::operator=(m_pSourceSurface->GetPosition());

        m_ptPosition.Offset(m_pSourceSurface->GetInset());
    }
    else
    {
        m_ptPosition.y = 0;
        m_ptPosition.x = 0;
    }

    m_pDestRect = 0;

    m_pClipRect = new RECT;

    m_bColourKey    = 0;
    m_nFlags        = DDBLTFAST_WAIT;
    m_bClipped      = 0;
    m_ptInset.y     = 0;
    m_ptInset.x     = 0;
    m_bSystemMemory = 1;
    m_bVisible      = 1;

    m_dwScrollColor                  = g_nDefaultScrollColour;
    m_dwFillColor                    = g_nDefaultFillColour;
    m_colorKey.dwColorSpaceLowValue  = g_nKeyColour;
    m_colorKey.dwColorSpaceHighValue = g_nKeyColour;

    caps.dwSize = sizeof(caps);

    m_hr = g_pSurfaceDDraw->GetCaps((LPDDCAPS)&caps, 0);

    m_dwSurfaceCaps = caps.ddsCaps.dwCaps;

    m_hr = 0;

    if (g_pSurfaceDDraw == 0)
    {
        // STRING: HYPERX 0x004be44c
        g_gameApp.Fatal("Cannot create CDDSurface: NULL DirectDraw object.");
    }

    if (g_pSurfaceFront == 0)
    {
        // STRING: HYPERX 0x004be41c
        g_gameApp.Fatal("Cannot create CDDSurface: NULL Front Surface.");
    }

    if (g_pSurfaceBack == 0)
    {
        // STRING: HYPERX 0x004be3ec
        g_gameApp.Fatal("Cannot create CDDSurface: NULL Back Surface.");
    }
}

// Slot 18 shares slot 17's address in five of six tables; slot 24 is KeyConfigDialog::SaveKeysAs.
void Texture::SetScrollColorMode(int nScrollColor)
{
    m_dwScrollColor = nScrollColor;
}

// Where the surface lives picks which pixel total it comes off.
// FUNCTION: HYPERX 0x00437050
Texture::~Texture()
{
    if (m_bSystemMemory != 0)
    {
        g_ptSurfacePixels.x -= m_ptSize.y * m_ptSize.x;
    }
    else
    {
        g_ptSurfacePixels.y -= m_ptSize.y * m_ptSize.x;
    }

    IDirectDrawSurface* const pSurface = m_pSurface;

    if (pSurface != 0)
    {
        pSurface->Release();

        m_pSurface = 0;
    }

    if (m_pDestRect != 0)
    {
        delete m_pDestRect;
        m_pDestRect = 0;
    }

    if (m_pClipRect != 0)
    {
        delete m_pClipRect;
        m_pClipRect = 0;
    }
}

struct IDirectDrawSurface;

#define KEY_WHITE 0xffffff

// System memory answers nought and adds to .x; video memory answers the pixel count and adds to .y.
// FUNCTION: HYPERX 0x004370e0
int Texture::AllocSurface(SurfacePoint size, int bColourKey, int bSystemMemory)
{
    DDrawCaps caps;
    DDSCAPS   surfaceCaps;

    m_bColourKey    = bColourKey;
    m_ptSize        = size;
    m_bSystemMemory = bSystemMemory;

    if (m_pClipRect != 0)
    {
        m_pClipRect->top  = 0;
        m_pClipRect->left = 0;

        m_pClipRect->right = m_ptSize.x - 1;

        m_pClipRect->bottom = m_ptSize.y - 1;
    }

    caps.dwSize = sizeof(caps);

    m_hr = g_pSurfaceDDraw->GetCaps((LPDDCAPS)&caps, 0);

    if (m_hr != 0)
    {
        return 0;
    }

    m_dwSurfaceCaps = caps.ddsCaps.dwCaps;

    if ((caps.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN) == 0)
    {
        m_bSystemMemory = 1;
    }

    m_pSurface = CreateSurface(m_ptSize.x, m_ptSize.y, m_bColourKey, m_bSystemMemory);

    if (m_pSurface == 0)
    {
        return 0;
    }

    m_hr = m_pSurface->GetCaps(&surfaceCaps);

    if (m_hr != 0)
    {
        return 0;
    }

    m_bSystemMemory = (0 < (surfaceCaps.dwCaps & DDSCAPS_SYSTEMMEMORY));

    if (m_bSystemMemory != 0)
    {
        g_ptSurfacePixels.x += m_ptSize.y * m_ptSize.x;

        return 0;
    }

    g_ptSurfacePixels.y += m_ptSize.y * m_ptSize.x;

    return m_ptSize.y * m_ptSize.x;
}

// FUNCTION: HYPERX 0x00437240
void Texture::SetFillColor(int nFillColor)
{
    m_dwFillColor = nFillColor;
}

// FUNCTION: HYPERX 0x00437250
void Texture::SetColorMode(int nScrollColor)
{
    m_dwScrollColor = nScrollColor;
}

// FUNCTION: HYPERX 0x00437260
int Texture::OnSurfaceLost()
{
    IDirectDrawSurface* const pSource = m_pSurface;

    if (pSource != 0)
    {
        m_hr = pSource->Restore();

        if (m_hr != 0)
        {
            // STRING: HYPERX 0x004be480
            g_gameApp.Fatal("CDDSurface::Restore Surface Failed %d!",
                            m_hr & DDRAW_ERROR_CODE_MASK);
        }
    }

    return RestoreContents();
}

// The key is the surface's own pixel for white, so it works at any depth.
// FUNCTION: HYPERX 0x004372a0
IDirectDrawSurface* Texture::CreateSurface(int nWidth, int nHeight, int bColourKey, int bSystemMemory)
{
    DDSURFACEDESC desc;

    IDirectDrawSurface* pSurface = 0;

    memset(&desc, 0, sizeof(desc));

    desc.dwSize         = sizeof(desc);
    desc.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    if (bSystemMemory != 0)
    {
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    }

    desc.dwHeight = nHeight;
    desc.dwWidth  = nWidth;

    m_hr = g_pSurfaceDDraw->CreateSurface(&desc, &pSurface, 0);

    if (m_hr < 0)
    {
        g_gameApp.Error(
            // STRING: HYPERX 0x004be4a8
            "CDDSurface::CreateSurface FAILED, rc = %d!",
            m_hr & DDRAW_ERROR_CODE_MASK);

        return 0;
    }

    if (bColourKey != 0)
    {
        m_dwFillColor = KEY_WHITE;

        m_nFlags |= DDBLTFAST_SRCCOLORKEY;

        if (g_bBackBufferColourKey == 0)
        {
            m_colorKey.dwColorSpaceHighValue = DDraw::ColourToPixel(pSurface, KEY_WHITE);
            m_colorKey.dwColorSpaceLowValue  = m_colorKey.dwColorSpaceHighValue;

            m_hr = pSurface->SetColorKey(DDCKEY_SRCBLT, &m_colorKey);
        }
    }

    return pSurface;
}

// FUNCTION: HYPERX 0x004373b0
void Texture::DispatchRect(int nLeft, int nTop, int nRight, int nBottom)
{
    TextureRect rect;

    rect.nLeft   = nLeft;
    rect.nTop    = nTop;
    rect.nRight  = nRight;
    rect.nBottom = nBottom;

    SetOffset(rect);
}

// A high-edge nudge subtracts the size, so nudging the right edge moves the origin left.
// FUNCTION: HYPERX 0x004373f0
void Texture::SetOffset(TextureRect rect)
{
    Point ptExtent;

    Point ptInset = GetInset();

    int nX = ptInset.x;
    int nY = ptInset.y;

    ptExtent.Set(0, 0);

    if (m_pSourceSurface != 0)
    {
        m_ptPosition.Point::operator=(m_pSourceSurface->GetPosition());

        ptInset = m_pSourceSurface->GetInset();

        nX = ptInset.x;
        nY = ptInset.y;

        ptExtent = m_pSourceSurface->GetSize();
    }

    if (rect.nLeft != 0)
    {
        m_ptPosition.x += nX + rect.nLeft;
    }

    if (rect.nTop != 0)
    {
        m_ptPosition.y += rect.nTop + nY;
    }

    if (rect.nRight != 0)
    {
        const int nSpan = rect.nRight - m_ptSize.x;

        m_ptPosition.x += (nSpan - nX) + ptExtent.x;
    }

    if (rect.nBottom != 0)
    {
        m_ptPosition.y += ((ptExtent.y - m_ptSize.y) - nY) + rect.nBottom;
    }
}

// A lost screen is the only error handled; a failed restore is fatal.
// FUNCTION: HYPERX 0x004374e0
int Texture::Draw()
{
    if (m_bVisible != 0)
    {
        m_hr = g_pSurfaceBack->BltFast(
            (DWORD)m_ptPosition.x, (DWORD)m_ptPosition.y, m_pSurface, m_pClipRect, (DWORD)m_nFlags);

        if (m_hr == DDERR_SURFACELOST)
        {
            m_hr = g_pSurfaceFront->Restore();

            if (m_hr != 0)
            {
                // STRING: HYPERX 0x004be4d4
                g_gameApp.Fatal("CDDSurface::Draw Primary Surface Restore failed!\n%s\n",
                                DDraw::ErrorToString(m_hr));
            }

            m_hr = OnSurfaceLost();
        }
        else
        {
            return 1;
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x00437570
void Texture::SetClipRect(int nClipped, Point pt)
{
    m_bClipped  = nClipped;
    m_ptInset.x = pt.x;
    m_ptInset.y = pt.y;
}

// FUNCTION: HYPERX 0x004375a0
int Texture::BltFill()
{
    RECT rc;

    if (m_pSurface == 0)
    {
        return 0;
    }

    m_bltfx.dwFillColor = m_dwFillColor;

    if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) != 0)
    {
        m_hr = m_pSurface->Blt(m_pDestRect, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
    }
    else
    {
        SetRect(&rc, m_ptPosition.x, m_ptPosition.y, m_ptSize.x + m_ptPosition.x - 1, m_ptSize.y + m_ptPosition.y - 1);

        m_hr = g_pSurfaceBack->Blt(&rc, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
    }

    if (m_hr == DDERR_SURFACELOST)
    {
        OnSurfaceLost();
    }

    return m_hr == 0;
}

// FUNCTION: HYPERX 0x00437650
int Texture::BltFillColour(unsigned int nColour)
{
    RECT rc;

    m_bltfx.dwFillColor = DDraw::ColourToPixel(m_pSurface, nColour);

    if (m_pSurface == 0)
    {
        return 0;
    }

    if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) != 0)
    {
        m_hr = m_pSurface->Blt(m_pDestRect, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
    }
    else
    {
        SetRect(&rc, m_ptPosition.x, m_ptPosition.y, m_ptSize.x + m_ptPosition.x - 1, m_ptSize.y + m_ptPosition.y - 1);

        m_hr = g_pSurfaceBack->Blt(&rc, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
    }

    if (m_hr == DDERR_SURFACELOST)
    {
        OnSurfaceLost();
    }

    return m_hr == 0;
}

// A clipped surface answers the clip origin's pixel.
// FUNCTION: HYPERX 0x00437710
unsigned char* Texture::LockAndGetPitch(int* pnPitch)
{
    DDSURFACEDESC desc;

    desc.dwSize = sizeof(desc);

    if (m_pSurface != 0)
    {
        m_hr = m_pSurface->Lock(0, &desc, DDLOCK_WAIT, 0);

        while (m_hr == DDERR_SURFACELOST)
        {
            m_pSurface->Restore();

            m_hr = m_pSurface->Lock(0, &desc, DDLOCK_WAIT, 0);
        }
    }

    if (m_hr != 0 || m_pSurface == 0)
    {
        // STRING: HYPERX 0x004be50c
        g_gameApp.Fatal("CDDSurface::Lock failed %d!", m_hr & DDRAW_ERROR_CODE_MASK);
    }

    *pnPitch = desc.lPitch;

    if (m_bClipped != 0)
    {
        return m_ptInset.y * desc.lPitch + m_ptInset.x + (unsigned char*)desc.lpSurface;
    }

    return (unsigned char*)desc.lpSurface;
}

// FUNCTION: HYPERX 0x004377e0
void Texture::Unused(int, int, int)
{
}

int g_nKeyColour           = 255;
int g_nDefaultScrollColour = 251;
int g_nDefaultFillColour   = 248;
