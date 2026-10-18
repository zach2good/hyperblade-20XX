#include "common/compat.h"
#include "graphics/hudlayer.h"
#include "globals.h"
#include "graphics/viewextent.h"
#include <algorithm>
#include <cstring>
#include <vector>

// The HUD was laid out for 480 lines; it keeps about that many however tall the window is.
constexpr auto HUD_LINES = 480;

namespace
{

struct DrawTarget
{
    unsigned char* pBits;
    int            nPitch;
    int            nWidth;
    int            nHeight;
};

std::vector<unsigned char> s_layerPixels;

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

    if (!s_layerPixels.empty() && nWidth == s_layer.nWidth && nHeight == s_layer.nHeight)
    {
        return;
    }

    s_layerPixels.assign((size_t)(nWidth * nHeight), 0);

    s_layer.pBits   = s_layerPixels.data();
    s_layer.nPitch  = nWidth;
    s_layer.nWidth  = nWidth;
    s_layer.nHeight = nHeight;
}

DrawTarget Current()
{
    return DrawTarget{ g_pBackBufferBitsCopy, g_nSurfacePitchCopy, g_nBackBufferWidth, g_nBackBufferHeight };
}

void Use(const DrawTarget& target)
{
    g_pBackBufferBitsCopy = target.pBits;
    g_nSurfacePitchCopy   = target.nPitch;
    g_nBackBufferWidth    = target.nWidth;
    g_nBackBufferHeight   = target.nHeight;
}

} // namespace

int HudLayer::Scale()
{
    return s_nScale;
}

void HudLayer::Enter()
{
    if (s_bEntered != 0 || g_pBackBufferBitsCopy == nullptr || g_nBackBufferWidth <= 0 || g_nBackBufferHeight <= 0)
    {
        return;
    }

    s_backBuffer = Current();

    Fit(s_backBuffer);

    if (s_layer.pBits == nullptr)
    {
        return;
    }

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
    if (s_layer.pBits != nullptr)
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

    return bFresh && s_layer.pBits != nullptr;
}

ViewExtent HudLayer::ToBackBuffer(ViewExtent extent)
{
    ViewExtent scaled;

    scaled.x = s_nOriginX + extent.x * s_nScale;
    scaled.y = s_nOriginY + extent.y * s_nScale;

    return scaled;
}
