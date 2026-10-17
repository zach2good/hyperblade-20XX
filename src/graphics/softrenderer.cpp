#include "common/compat.h"
#include "graphics/bitmapformat.h"
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
#include <vector>

namespace
{

std::vector<unsigned char> s_backBuffer;

} // namespace

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

    g_pszDibOrientationName = g_szTopDownDibName;
    g_bBottomUpDIB          = 0;

    s_backBuffer.assign((size_t)g_nBackBufferPixelCount, 0);

    g_pBackBufferBitsCopy = s_backBuffer.data();
    g_nBackBufferWidth    = g_nPrevWidth;
    g_nBackBufferHeight   = g_nPrevHeight;

    Graphics::Initialize(1);

    if (g_pRenderer != 0)
    {
        g_pRenderer->LayoutSurfaces();
    }

    return bWidthRounded;
}

// FUNCTION: HYPERX 0x0044b7c0
void SoftRenderer::FreeBackBuffer()
{
    s_backBuffer = {};

    g_pBackBufferBitsCopy = 0;
}

// FUNCTION: HYPERX 0x0044f480
RLESprite* SoftRenderer::LoadRLESprite(const char* pszName)
{
    PathString strPath(MAX_PATH_LENGTH);

    // STRING: HYPERX 0x004bc45c
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, pszName, "bmp");

    BitmapInfoHeader* const pDib = GDI::ReadBitmapFile(strPath.m_str.m_pchData);

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
