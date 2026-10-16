#include "common/compat.h"
#include "graphics/hudlayer.h"
#include "globals.h"
#include "graphics/viewextent.h"
#include <algorithm>
#include <string.h>

// The HUD was laid out for 480 lines; it keeps about that many however tall the window is.
constexpr auto HUD_LINES = 480;

// The key's colour in the layer's own table, so GDI text never matches it.
constexpr auto KEY_COLOUR = RGBQUAD{ 0xfd, 0x00, 0xfe, 0 };

namespace
{

struct DrawTarget
{
    unsigned char* pBits;
    int            nPitch;
    int            nWidth;
    int            nHeight;
    HDC            hSoftDC;
    HDC            hTextDC;
};

HDC     s_hLayerDC     = 0;
HBITMAP s_hLayerBitmap = 0;

// The back buffer while the layer is entered, so the layer can be fitted and composited to it.
DrawTarget s_backBuffer = {};
DrawTarget s_layer      = {};
int        s_bEntered   = 0;
int        s_nScale     = 1;
int        s_nOriginX   = 0;
int        s_nOriginY   = 0;
int        s_bFresh     = 0;

// Recreates the layer when the back buffer it covers has changed size.
void Fit(const DrawTarget& backBuffer)
{
    s_nScale = (std::max)(1, backBuffer.nHeight / HUD_LINES);

    const auto nWidth  = (backBuffer.nWidth / s_nScale) & ~3;
    const auto nHeight = backBuffer.nHeight / s_nScale;

    s_nOriginX = (backBuffer.nWidth - nWidth * s_nScale) / 2;
    s_nOriginY = (backBuffer.nHeight - nHeight * s_nScale) / 2;

    if (s_hLayerBitmap != 0 && nWidth == s_layer.nWidth && nHeight == s_layer.nHeight)
    {
        return;
    }

    if (s_hLayerDC == 0)
    {
        s_hLayerDC = CreateCompatibleDC(backBuffer.hSoftDC);
    }

    if (s_hLayerBitmap != 0)
    {
        DeleteObject(s_hLayerBitmap);
    }

    struct
    {
        BITMAPINFOHEADER header;
        RGBQUAD          colours[256];
    } info = {};

    info.header.biSize     = sizeof(info.header);
    info.header.biWidth    = nWidth;
    info.header.biHeight   = -nHeight;
    info.header.biPlanes   = 1;
    info.header.biBitCount = 8;

    void* pBits    = 0;
    s_hLayerBitmap = CreateDIBSection(s_hLayerDC, (BITMAPINFO*)&info, DIB_RGB_COLORS, &pBits, 0, 0);

    SelectObject(s_hLayerDC, s_hLayerBitmap);

    s_layer.pBits   = (unsigned char*)pBits;
    s_layer.nPitch  = nWidth;
    s_layer.nWidth  = nWidth;
    s_layer.nHeight = nHeight;
    s_layer.hSoftDC = s_hLayerDC;
    s_layer.hTextDC = s_hLayerDC;
}

DrawTarget Current()
{
    return DrawTarget{ g_pBackBufferBitsCopy, g_nSurfacePitchCopy, g_nBackBufferWidth, g_nBackBufferHeight, g_hSoftBackDC, g_hTextDC };
}

void Use(const DrawTarget& target)
{
    g_pBackBufferBitsCopy = target.pBits;
    g_nSurfacePitchCopy   = target.nPitch;
    g_nBackBufferWidth    = target.nWidth;
    g_nBackBufferHeight   = target.nHeight;
    g_hSoftBackDC         = target.hSoftDC;
    g_hTextDC             = target.hTextDC;
}

} // namespace

int HudLayer::Scale()
{
    return s_nScale;
}

void HudLayer::Enter()
{
    if (s_bEntered != 0 || g_hSoftBackDC == 0 || g_nBackBufferWidth <= 0 || g_nBackBufferHeight <= 0)
    {
        return;
    }

    s_backBuffer = Current();

    Fit(s_backBuffer);

    if (s_layer.pBits == 0)
    {
        return;
    }

    // The palette can change between frames, so the layer takes the back buffer's table each time.
    RGBQUAD aColours[256];

    const auto nColours = GetDIBColorTable(s_backBuffer.hSoftDC, 0, 256, aColours);

    aColours[KEY_INDEX] = KEY_COLOUR;
    SetDIBColorTable(s_hLayerDC, 0, (std::max)(nColours, (UINT)KEY_INDEX + 1), aColours);
    Use(s_layer);

    s_bEntered = 1;
}

void HudLayer::Leave()
{
    if (s_bEntered == 0)
    {
        return;
    }

    Use(s_backBuffer);

    s_bEntered = 0;
}

bool HudLayer::IsEntered()
{
    return s_bEntered != 0;
}

void HudLayer::Clear()
{
    if (s_layer.pBits != 0)
    {
        memset(s_layer.pBits, KEY_INDEX, (size_t)s_layer.nPitch * s_layer.nHeight);

        s_bFresh = 1;
    }
}

const HudLayer::Layer& HudLayer::Pixels()
{
    static Layer s_pixels;

    s_pixels = Layer{ s_layer.pBits, s_layer.nPitch, s_layer.nWidth, s_layer.nHeight, s_nScale, s_nOriginX, s_nOriginY };

    return s_pixels;
}

bool HudLayer::TakeFrame()
{
    const auto bFresh = s_bFresh != 0;

    s_bFresh = 0;

    return bFresh && s_layer.pBits != 0;
}

ViewExtent HudLayer::ToBackBuffer(ViewExtent extent)
{
    ViewExtent scaled;

    scaled.x = s_nOriginX + extent.x * s_nScale;
    scaled.y = s_nOriginY + extent.y * s_nScale;

    return scaled;
}

int HudLayer::NearestIndex(HPALETTE hPalette, COLORREF colour)
{
    PALETTEENTRY aEntries[256];

    const auto nEntries = (int)GetPaletteEntries(hPalette, 0, 256, aEntries);

    if (nEntries == 0)
    {
        return GetNearestPaletteIndex(hPalette, colour);
    }

    auto nBest     = 0;
    auto nBestDist = 0x7fffffff;

    for (int i = 0; i < nEntries; i++)
    {
        if (i == KEY_INDEX)
        {
            continue;
        }

        const int dr    = (int)aEntries[i].peRed - GetRValue(colour);
        const int dg    = (int)aEntries[i].peGreen - GetGValue(colour);
        const int db    = (int)aEntries[i].peBlue - GetBValue(colour);
        const int nDist = dr * dr + dg * dg + db * db;

        if (nDist < nBestDist)
        {
            nBestDist = nDist;
            nBest     = i;
        }
    }

    return nBest;
}
