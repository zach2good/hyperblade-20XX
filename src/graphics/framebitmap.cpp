#ifdef HYPERX_V100

#include "common/compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <ddraw.h>
#include "graphics/framebitmap.h"
#include "graphics/graphicsglobals.h"
#include "graphics/ddrawapi.h"

// The DirectX 3 DDCAPS, which is what v1.0.0.0 hands GetCaps.
#define DX3_DDCAPS_SIZE 0x13c

// FUNCTION: HYPERX100 0x00444860
FrameBitmap::FrameBitmap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
: CDDBitMap(pDDraw, pPrimary, pBack)
{
    ClearState();
}

// FUNCTION: HYPERX100 0x00444930
FrameBitmap::FrameBitmap()
{
    ClearState();
}

// FUNCTION: HYPERX100 0x004449b0
void FrameBitmap::ClearState()
{
    m_ppFrames    = 0;
    m_nFrameCount = 0;
    m_nFrame      = 0;
}

// FUNCTION: HYPERX100 0x004449d0
FrameBitmap::~FrameBitmap()
{
    // The surface is the current frame's, so the base must not release it.
    m_pSurface = 0;

    if (m_ppFrames != 0)
    {
        for (int i = 0; i < m_nFrameCount; i++)
        {
            if (m_ppFrames[i] != 0)
            {
                delete m_ppFrames[i];

                m_ppFrames[i] = 0;
            }
        }

        delete[] m_ppFrames;

        m_ppFrames = 0;
    }
}

// Answers the number of frames tried, or nought when the table or the caps query fails.
// FUNCTION: HYPERX100 0x00444ad0
unsigned char FrameBitmap::LoadFrames(const char* pszBaseName, int nCount, int bColourKey, int)
{
    char          szName[256];
    DDCAPS        caps;
    unsigned char i;

    m_nFrameCount = (unsigned char)nCount;
    m_ppFrames    = new CDDBitMap*[nCount];

    if (m_ppFrames == 0)
    {
        return 0;
    }

    if (bColourKey != 0)
    {
        caps.dwSize = DX3_DDCAPS_SIZE;

        m_hr = g_pSurfaceDDraw->GetCaps(&caps, 0);

        if (m_hr != 0)
        {
            return 0;
        }

        m_bSystemMemory = 1;

        if ((caps.dwCaps & DDCAPS_COLORKEYHWASSIST) == 0)
        {
            m_bSystemMemory = 0;
        }
    }

    for (i = 0; i < nCount; i++)
    {
        sprintf(szName, "%s%02d", pszBaseName, i + 1);

        m_ppFrames[i] = new CDDBitMap(this);

        if (m_ppFrames[i] != 0)
        {
            m_ppFrames[i]->LoadFromFile(szName, bColourKey, m_bSystemMemory);

            if (bColourKey != 0)
            {
                m_dwFillColor = DDraw::ColourToPixel(m_ppFrames[i]->GetSurface(), 0xffffff);
            }
        }
    }

    m_bltfx.dwFillColor = m_dwFillColor;

    if (m_ppFrames[0] != 0)
    {
        m_ptSize = m_ppFrames[0]->GetSize();
    }

    if (m_pClipRect != 0)
    {
        m_pClipRect->top    = 0;
        m_pClipRect->left   = m_pClipRect->top;
        m_pClipRect->right  = m_ptSize.x - 1;
        m_pClipRect->bottom = m_ptSize.y - 1;
    }

    return i;
}

// Each frame restores its own surface, and the answer is always one.
// FUNCTION: HYPERX100 0x00444d50
int FrameBitmap::RestoreContents()
{
    if (m_ppFrames != 0)
    {
        for (int i = 0; i < m_nFrameCount; i++)
        {
            if (m_ppFrames[i] != 0)
            {
                m_ppFrames[i]->RestoreContents();
            }
        }
    }

    return 1;
}

// The back buffer is unlocked until it reports it is not locked, but locked again only once.
// FUNCTION: HYPERX100 0x00444db0
int FrameBitmap::BltFrame(RECT rcDest)
{
    DDSURFACEDESC ddsd;

    int bResult = 0;

    if (m_bVisible == 0)
    {
        return bResult;
    }

    if (m_pSurface == 0)
    {
        return 0;
    }

    short nUnlocks = 0;

    m_hr = g_pSurfaceBack->Unlock(0);

    if (m_hr == DDERR_SURFACELOST)
    {
        RestoreContents();

        return 0;
    }

    if (m_hr != DDERR_NOTLOCKED)
    {
        if (m_hr != 0)
        {
            return 0;
        }

        nUnlocks = 1;

        for (;;)
        {
            m_hr = g_pSurfaceBack->Unlock(0) != DDERR_NOTLOCKED;

            if (m_hr == 0)
            {
                break;
            }

            nUnlocks++;
        }
    }

    m_hr = g_pSurfaceBack->Blt(&rcDest, m_pSurface, m_pClipRect, DDBLT_WAIT | DDBLT_KEYSRC, &m_bltfx);

    if (m_hr == DDERR_SURFACELOST)
    {
        RestoreContents();
    }

    bResult = m_hr == 0;

    if (nUnlocks != 0)
    {
        ddsd.dwSize = sizeof(ddsd);

        m_hr = g_pSurfaceBack->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
    }

    return bResult;
}

// FUNCTION: HYPERX100 0x00444f10
int FrameBitmap::SetFrame(unsigned char nFrame, RECT rcDest)
{
    if (m_bVisible != 0)
    {
        m_nFrame   = nFrame;
        m_pSurface = m_ppFrames[nFrame]->GetSurface();

        return BltFrame(rcDest);
    }

    return 0;
}

// GDI::CreatePaletteFromDIB without the reserved entries: every flag is zero.
// TODO: The owner is a guess. It follows SetFrame in the image and nothing calls it.
// FUNCTION: HYPERX100 0x00444f70
HPALETTE FrameBitmap::CreatePaletteFromDIB(BITMAPINFO* pbmi, int* pcColors)
{
    HPALETTE hPalette = 0;

    if (pbmi->bmiHeader.biBitCount <= 8)
    {
        *pcColors = 1 << pbmi->bmiHeader.biBitCount;
    }
    else
    {
        *pcColors = 0;
    }

    if (*pcColors != 0)
    {
        LOGPALETTE* const pLog = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + *pcColors * sizeof(PALETTEENTRY));

        pLog->palVersion    = 0x300;
        pLog->palNumEntries = (WORD)*pcColors;

        for (int i = 0; i < *pcColors; i++)
        {
            pLog->palPalEntry[i].peRed   = pbmi->bmiColors[i].rgbRed;
            pLog->palPalEntry[i].peGreen = pbmi->bmiColors[i].rgbGreen;
            pLog->palPalEntry[i].peBlue  = pbmi->bmiColors[i].rgbBlue;
            pLog->palPalEntry[i].peFlags = 0;
        }

        hPalette = CreatePalette(pLog);

        free(pLog);
    }

    return hPalette;
}

#endif
