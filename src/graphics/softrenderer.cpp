#include "common/compat.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "graphics/gdi.h"
#include "renderer.h"
#include "ui/infobarbase.h"
#include "rlesprite.h"
#include "common/pathstring.h"
#include <stdlib.h>
#include "app/gameapp.h"
#include "softtex.h"
#include "graphics/graphics.h"
#include "graphics/softrenderer.h"

// The width rounds up to a multiple of four; the answer is only whether that changed it.
// FUNCTION: HYPERX 0x0044ba50
int SoftRenderer::ResizeBackBuffer(int* pWidth, int* pHeight)
{
    int bWidthRounded = 0;

    if (*pWidth == g_nPrevWidth && *pHeight == g_nPrevHeight)
    {
        return 0;
    }

    g_nPrevWidth            = (*pWidth + 3) & ~3;
    g_nPrevHeight           = *pHeight;
    g_nBackBufferPixelCount = g_nPrevHeight * g_nPrevWidth;

    if (g_nPrevWidth != *pWidth)
    {
        *pWidth       = g_nPrevWidth;
        bWidthRounded = 1;
    }

    if (g_cUseSoftRenderer == 0 && g_bBackBufferReady != 0)
    {
        if (g_pRenderer != 0)
        {
            g_pRenderer->LayoutSurfaces();
        }
    }
    else if (g_hSoftBackDC != 0)
    {
        if (g_hSoftBackBitmap != 0)
        {
            SelectObject(g_hSoftBackDC, g_hSoftBackOldBitmap);
            DeleteObject(g_hSoftBackBitmap);
        }

        g_pszDibOrientationName = g_szTopDownDibName;
        g_bBottomUpDIB          = 0;

        g_bmiSoftBackBuffer.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        g_bmiSoftBackBuffer.bmiHeader.biWidth         = g_nPrevWidth;
        g_bmiSoftBackBuffer.bmiHeader.biHeight        = -g_nPrevHeight;
        g_bmiSoftBackBuffer.bmiHeader.biPlanes        = 1;
        g_bmiSoftBackBuffer.bmiHeader.biBitCount      = 8;
        g_bmiSoftBackBuffer.bmiHeader.biCompression   = BI_RGB;
        g_bmiSoftBackBuffer.bmiHeader.biSizeImage     = 0;
        g_bmiSoftBackBuffer.bmiHeader.biXPelsPerMeter = 0;
        g_bmiSoftBackBuffer.bmiHeader.biYPelsPerMeter = 0;
        g_bmiSoftBackBuffer.bmiHeader.biClrUsed       = 0;
        g_bmiSoftBackBuffer.bmiHeader.biClrImportant  = 0;

        g_hSoftBackBitmap = CreateDIBSection(g_hSoftBackDC,
                                             (BITMAPINFO*)&g_bmiSoftBackBuffer,
                                             DIB_RGB_COLORS,
                                             (void**)&g_pBackBufferBitsCopy,
                                             0,
                                             0);

        if (g_hSoftBackBitmap == 0)
        {
            // STRING: HYPERX 0x004c2dac
            g_gameApp.Fatal("Error allocating graphics display buffer!");
        }

        g_nBackBufferWidth  = g_nPrevWidth;
        g_nBackBufferHeight = g_nPrevHeight;

        g_hSoftBackOldBitmap = SelectObject(g_hSoftBackDC, g_hSoftBackBitmap);

        Graphics::Initialize(1);

        SetBkMode(g_hSoftBackDC, TRANSPARENT);
        SetTextColor(g_hSoftBackDC, GetNearestColor(g_hSoftBackDC, RGB(0, 255, 0)));

        if (g_pRenderer != 0)
        {
            g_pRenderer->LayoutSurfaces();
        }
    }

    return bWidthRounded;
}

// FUNCTION: HYPERX 0x0044f480
RLESprite* __cdecl SoftRenderer::LoadRLESprite(const char* pszName)
{
    PathString strPath(MAX_PATH);

    // STRING: HYPERX 0x004bc45c
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, pszName, "bmp");

    BITMAPINFOHEADER* const pDib = GDI::ReadBitmapFile(strPath.m_str.m_pchData);

    if (pDib == 0)
    {
        return 0;
    }

    RLESprite* const pTexture = SoftTex::CompressToRLE(pDib);

    free(pDib);

    return pTexture;
}

char g_szTopDownDibName[12] = "TopDownDIB ";

// STRING: HYPERX 0x004c2ca8
char* g_pszDibOrientationName = "BottomUpDIB";

// 50x50, 8 bits: the back buffer the original builds before it knows the display mode.
SoftBackBitmapInfo g_bmiSoftBackBuffer = { { 40, 50, 50, 1, 8 } };
