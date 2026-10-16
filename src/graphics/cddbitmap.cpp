#include "common/compat.h"
#include "cddbitmap.h"
#include "common/pathstring.h"
#include "graphics/texture.h"
#include "globals.h"
#include "texture.h"
#include <new.h>
#include <ddraw.h>
#include "app/gameapp.h"
#include "cddtextsurface.h"
#include "math/point.h"
#include "graphics/ddrawapi.h"

// FUNCTION: HYPERX 0x00437f30
CDDBitMap::CDDBitMap()
{
    ClearBounds();
}

// FUNCTION: HYPERX 0x00437fa0
CDDBitMap::CDDBitMap(Texture* pSource)
: Texture(pSource)
{
    ClearBounds();
}

// FUNCTION: HYPERX 0x00437ff0
CDDBitMap::CDDBitMap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
: Texture(pDDraw, pPrimary, pBack)
{
    ClearBounds();
}

// FUNCTION: HYPERX 0x00438050
void CDDBitMap::ClearBounds()
{
    m_pName = 0;

    SetRect(&m_rcBounds, 0, 0, 0, 0);

    m_pOverlay = 0;
}

// FUNCTION: HYPERX 0x00438080
CDDBitMap::~CDDBitMap()
{
    if (m_pOverlay != 0)
    {
        delete m_pOverlay;

        m_pOverlay = 0;
    }

    if (m_pName != 0)
    {
        delete m_pName;

        m_pName = 0;
    }

    m_pDestRect = 0;
}

// A failed name allocation stores null and the load runs anyway.
// FUNCTION: HYPERX 0x00438110
int CDDBitMap::LoadFromFile(const char* pszName, int bColourKey, int bSystemMemory)
{
    m_pName = new String(pszName);

    return CreateFromHBitmap(g_gameApp.LoadGDIBitmap(pszName), bColourKey, bSystemMemory);
}

// A bitmap GetObjectA rejects is deleted here, since nothing else holds it.
// FUNCTION: HYPERX 0x004381a0
int CDDBitMap::CreateFromHBitmap(HBITMAP hBitmap, int bColourKey, int bSystemMemory)
{
    BITMAP bm;
    int    nResult;

    if (hBitmap != 0)
    {
        if (GetObjectA(hBitmap, sizeof(BITMAP), &bm) != 0)
        {
            m_ptSize.x = bm.bmWidth;
            m_ptSize.y = bm.bmHeight;

            nResult = Texture::AllocSurface(m_ptSize, bColourKey, bSystemMemory);

            if (m_pSurface != 0)
            {
                CopyBitmap(hBitmap);
            }

            return nResult;
        }

        DeleteObject(hBitmap);

        return 0;
    }

    // STRING: HYPERX 0x004be5d0
    g_gameApp.Error("CDDBitMap::Create FAILED! No BitMap loaded!\n");

    return 0;
}

// Ignores case when comparing the name.
// FUNCTION: HYPERX 0x00438230
void CDDBitMap::SetFile(const char* pszName, int bColourKey)
{
    String*     pName      = m_pName;
    const int   nLength    = pName->GetHeader()->nDataLength;
    char* const pszCurrent = pName->Reserve(nLength);

    if (lstrcmpiA(pszCurrent, pszName) == 0)
    {
        return;
    }

    if (m_pSurface != 0)
    {
        m_pSurface->Release();

        m_pSurface = 0;
    }

    pName = m_pName;

    if (pName != 0)
    {
        delete pName;

        m_pName = 0;
    }

    LoadFromFile(pszName, bColourKey, m_bSystemMemory);
}

// FUNCTION: HYPERX 0x004382b0
int CDDBitMap::Draw()
{
    if (m_bVisible != 0 && m_pSurface != 0)
    {
        if (m_pOverlay != 0)
        {
            m_pOverlay->Draw();
        }

        return Texture::Draw();
    }

    return 0;
}

// FUNCTION: HYPERX 0x004382e0
int CDDBitMap::SetTextOverlay(const char* pszName, int nSize, const char* pszText, int bColourKey)
{
    int nResult;

    m_pOverlay = new CDDTextSurface;

    if (m_pOverlay == 0)
    {
        return 0;
    }

    nResult = m_pOverlay->SetFont(pszName, nSize, pszText, bColourKey, m_bSystemMemory);

    m_pOverlay->OnSurfaceLost();

    return nResult;
}

// FUNCTION: HYPERX 0x00438390
void CDDBitMap::SetPosition(Point pt)
{
    m_ptPosition.Point::operator=(pt);

    if (m_pOverlay != 0)
    {
        pt.Zero();

        pt.x = ((m_ptSize.x - m_pOverlay->GetSize().x) >> 1) + m_ptPosition.x;
        pt.y = m_ptPosition.y + m_ptSize.y + 2;

        if (m_pOverlay != 0)
        {
            m_pOverlay->SetPosition(pt);
        }
    }
}

// Returns success whatever the reload did.
// FUNCTION: HYPERX 0x00438410
int CDDBitMap::OnSurfaceLost()
{
    Texture::OnSurfaceLost();

    if (m_pOverlay != 0)
    {
        m_pOverlay->OnSurfaceLost();
    }

    Draw();

    return 1;
}

// FUNCTION: HYPERX 0x00438440
int CDDBitMap::RestoreContents()
{
    HBITMAP pBitmap = 0;

    if (m_pName != 0)
    {
        pBitmap = g_gameApp.LoadGDIBitmap(*m_pName);
    }

    return CopyBitmap(pBitmap);
}

// The source colour key is the first pixel, masked to the surface's bit depth.
// FUNCTION: HYPERX 0x00438470
int CDDBitMap::CopyBitmap(HBITMAP hBitmap)
{
    HDC hdcSurface;

    if (hBitmap == 0 || m_pSurface == 0)
    {
        return 0;
    }

    m_hr = m_pSurface->GetDC(&hdcSurface);

    if (m_hr == DDERR_SURFACELOST)
    {
        m_hr = m_pSurface->Restore();

        if (m_hr != 0)
        {
            // STRING: HYPERX 0x004be66c
            g_gameApp.Fatal("CDDBitMap::RestoreContents Surface Restore failed!\n%s\n",
                            DDraw::ErrorToString(m_hr));
        }

        m_hr = m_pSurface->GetDC(&hdcSurface);

        if (m_hr == DDERR_SURFACELOST)
        {
            // STRING: HYPERX 0x004be634
            g_gameApp.Fatal("CDDBitMap::RestoreContents Surface GetDC failed!\n%s\n",
                            DDraw::ErrorToString(m_hr));
        }
    }

    if (m_hr != 0)
    {
        return 0;
    }

    const HDC hdcBitmap = CreateCompatibleDC(hdcSurface);

    SelectObject(hdcBitmap, hBitmap);

    {
        RECT rc;

        SetRect(&rc, 0, 0, m_ptSize.x, m_ptSize.y);

        BitBlt(hdcSurface, rc.left, rc.top, rc.right, rc.bottom, hdcBitmap, 0, 0, SRCCOPY);
    }

    DeleteDC(hdcBitmap);

    m_pSurface->ReleaseDC(hdcSurface);

    if (m_bColourKey != 0)
    {
        DDSURFACEDESC desc;
        DDCOLORKEY    key;

        desc.dwSize = sizeof(desc);

        m_hr = m_pSurface->Lock(0, &desc, DDLOCK_WAIT, 0);

        if (m_hr == DDERR_SURFACELOST)
        {
            m_hr = m_pSurface->Restore();

            if (m_hr != 0)
            {
                // STRING: HYPERX 0x004be600
                g_gameApp.Fatal("CDDBitMap::RestoreBitMap Surface Restore failed %d!",
                                m_hr & DDRAW_ERROR_CODE_MASK);
            }
        }

        if (m_hr == 0)
        {
            key.dwColorSpaceLowValue  = *(unsigned int*)desc.lpSurface & ((1 << desc.ddpfPixelFormat.dwRGBBitCount) - 1);
            key.dwColorSpaceHighValue = key.dwColorSpaceLowValue;

            m_pSurface->Unlock(0);

            // Lock does not see what GDI wrote, so the key is read through GDI once unlocked.
            if (key.dwColorSpaceLowValue == 0)
            {
                key.dwColorSpaceLowValue  = DDraw::CornerColourKey(m_pSurface,
                                                                   desc.ddpfPixelFormat.dwRBitMask,
                                                                   desc.ddpfPixelFormat.dwGBitMask,
                                                                   desc.ddpfPixelFormat.dwBBitMask);
                key.dwColorSpaceHighValue = key.dwColorSpaceLowValue;
            }
        }

        m_pSurface->SetColorKey(DDCKEY_SRCBLT, &key);
    }

    DeleteObject(hBitmap);

    return 1;
}

// FUNCTION: HYPERX 0x00438650
void CDDBitMap::SetClipRect(int nClipped, Point pt)
{
    Texture::SetClipRect(nClipped, pt);

    if (m_bClipped != 0)
    {
        SetRect(&m_rcBounds, m_ptInset.x, m_ptInset.y, m_ptSize.x - m_ptInset.x, m_ptSize.y - m_ptInset.y);

        m_pDestRect = &m_rcBounds;
    }
    else
    {
        m_pDestRect = 0;
    }
}
