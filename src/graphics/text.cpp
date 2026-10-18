#include "graphics/text.h"
#include "common/pak.h"
#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

namespace
{

struct Glyph
{
    std::vector<unsigned char> mask; // One byte a pixel, set where the glyph covers at least half.
    int                        nWidth    = 0;
    int                        nHeight   = 0;
    int                        nOffsetX  = 0; // From the pen.
    int                        nOffsetY  = 0; // From the baseline.
    float                      flAdvance = 0.0f;
};

struct Face
{
    std::vector<unsigned char> file;
    stbtt_fontinfo             info   = {};
    bool                       bReady = false;
};

struct FontSize
{
    const Face*            pFace       = nullptr;
    float                  flScale     = 0.0f;
    int                    nAscent     = 0;
    int                    nCellHeight = 0;
    std::array<Glyph, 256> glyphs;
    std::array<bool, 256>  abBuilt = {};
};

Face MakeFace(std::vector<unsigned char> file)
{
    auto face = Face{ std::move(file) };

    face.bReady = !face.file.empty() && stbtt_InitFont(&face.info, face.file.data(), stbtt_GetFontOffsetForIndex(face.file.data(), 0)) != 0;

    return face;
}

// A font from the Windows fonts folder; empty where it is missing.
std::vector<unsigned char> LoadWindowsFont(const char* pszName)
{
    const auto path = [&]
    {
        const auto* const pszWindows = SDL_getenv("WINDIR");

        if (pszWindows == nullptr)
        {
            return std::string("C:\\Windows\\Fonts\\") + pszName;
        }

        return std::string(pszWindows) + "\\Fonts\\" + pszName;
    }();

    auto nSize = static_cast<size_t>(0);

    auto* const pData = SDL_LoadFile(path.c_str(), &nSize);

    if (pData == nullptr)
    {
        return {};
    }

    const auto file = std::vector<unsigned char>(static_cast<const unsigned char*>(pData), static_cast<const unsigned char*>(pData) + nSize);

    SDL_free(pData);

    return file;
}

// Pixel Operator, CC0, from the pak. Each draws crisply at a multiple of its design size.
const Face& PixelOperator()
{
    static const auto s_face = MakeFace(Pak::Read("fonts/PixelOperator.ttf"));

    return s_face;
}

const Face& PixelOperator8()
{
    static const auto s_face = MakeFace(Pak::Read("fonts/PixelOperator8.ttf"));

    return s_face;
}

// The menus' face: Microsoft Sans Serif, or Pixel Operator where Windows' fonts are missing.
const Face& MenuFace()
{
    static const auto s_face = MakeFace(LoadWindowsFont("micross.ttf"));

    if (s_face.bReady)
    {
        return s_face;
    }

    return PixelOperator();
}

std::map<std::tuple<const Face*, int, bool>, std::unique_ptr<FontSize> > s_sizes;

// A face at a size: by its em for a pixel font, which then lands on whole pixels, or by its full
// height in pixels otherwise.
FontSize* SizeFor(const Face& face, int nPixels, bool bEm)
{
    auto& pSize = s_sizes[{ &face, nPixels, bEm }];

    if (pSize != nullptr)
    {
        return pSize.get();
    }

    pSize = std::make_unique<FontSize>();

    pSize->pFace   = &face;
    pSize->flScale = [&]
    {
        if (bEm)
        {
            return stbtt_ScaleForMappingEmToPixels(&face.info, static_cast<float>(nPixels));
        }

        return stbtt_ScaleForPixelHeight(&face.info, static_cast<float>(nPixels));
    }();

    auto nAscent  = 0;
    auto nDescent = 0;
    auto nLineGap = 0;

    stbtt_GetFontVMetrics(&face.info, &nAscent, &nDescent, &nLineGap);

    pSize->nAscent     = static_cast<int>(std::lround(nAscent * pSize->flScale));
    pSize->nCellHeight = [&]
    {
        if (bEm)
        {
            return static_cast<int>(std::lround((nAscent - nDescent) * pSize->flScale));
        }

        return nPixels;
    }();

    return pSize.get();
}

FontSize* SizeOf(Text::Font font)
{
    return font.As<FontSize>();
}

// ponytail: bytes go to Unicode as Latin-1, so Windows-1252's 0x80 to 0x9f draw nothing.
const Glyph& GetGlyph(FontSize& size, unsigned char nChar)
{
    auto& glyph = size.glyphs[nChar];

    if (size.abBuilt[nChar])
    {
        return glyph;
    }

    size.abBuilt[nChar] = true;

    const auto& info = size.pFace->info;

    auto nAdvance = 0;
    auto nBearing = 0;

    stbtt_GetCodepointHMetrics(&info, nChar, &nAdvance, &nBearing);

    glyph.flAdvance = nAdvance * size.flScale;

    auto nX0 = 0;
    auto nY0 = 0;
    auto nX1 = 0;
    auto nY1 = 0;

    stbtt_GetCodepointBitmapBox(&info, nChar, size.flScale, size.flScale, &nX0, &nY0, &nX1, &nY1);

    glyph.nWidth   = nX1 - nX0;
    glyph.nHeight  = nY1 - nY0;
    glyph.nOffsetX = nX0;
    glyph.nOffsetY = nY0;

    if (glyph.nWidth <= 0 || glyph.nHeight <= 0)
    {
        return glyph;
    }

    glyph.mask.resize(static_cast<size_t>(glyph.nWidth * glyph.nHeight));

    stbtt_MakeCodepointBitmap(&info, glyph.mask.data(), glyph.nWidth, glyph.nHeight, glyph.nWidth, size.flScale, size.flScale, nChar);

    for (auto& nCoverage : glyph.mask)
    {
        nCoverage = static_cast<unsigned char>(nCoverage >= 128);
    }

    return glyph;
}

float Kerning(const FontSize& size, unsigned char nFirst, unsigned char nSecond)
{
    return stbtt_GetCodepointKernAdvance(&size.pFace->info, nFirst, nSecond) * size.flScale;
}

// Calls plot(x, y) for every pixel the text sets, clipped to the target.
template <class Plot>
void Rasterise(Text::Font font, int nWidth, int nHeight, int nX, int nY, const char* pszText, Plot plot)
{
    auto* const pSize = SizeOf(font);

    if (pSize == nullptr || pszText == nullptr)
    {
        return;
    }

    const auto nBaseline = nY + pSize->nAscent;

    auto flPen = static_cast<float>(nX);

    for (const auto* p = reinterpret_cast<const unsigned char*>(pszText); *p != 0; p++)
    {
        const auto& glyph = GetGlyph(*pSize, *p);

        const auto nLeft = static_cast<int>(std::floor(flPen)) + glyph.nOffsetX;
        const auto nTop  = nBaseline + glyph.nOffsetY;

        for (int y = 0; y < glyph.nHeight; y++)
        {
            const auto nRow = nTop + y;

            if (nRow < 0 || nRow >= nHeight)
            {
                continue;
            }

            for (int x = 0; x < glyph.nWidth; x++)
            {
                const auto nColumn = nLeft + x;

                if (glyph.mask[static_cast<size_t>(y * glyph.nWidth + x)] != 0 && nColumn >= 0 && nColumn < nWidth)
                {
                    plot(nColumn, nRow);
                }
            }
        }

        flPen += glyph.flAdvance;

        if (p[1] != 0)
        {
            flPen += Kerning(*pSize, p[0], p[1]);
        }
    }
}

} // namespace

Text::Font Text::Find(int nHeight)
{
    const auto& face = MenuFace();

    if (!face.bReady || nHeight <= 0)
    {
        return Font{};
    }

    return Font{ SizeFor(face, nHeight, false) };
}

// The HUD's small labels take Pixel Operator at its sixteen-pixel design; the larger ones, such as
// the clock, Pixel Operator 8 doubled.
Text::Font Text::FindSansSerif(int nHeight)
{
    constexpr auto kLargeHeight = 20;
    constexpr auto kDesignEm    = 16;

    const auto& face = [&]() -> const Face&
    {
        if (nHeight >= kLargeHeight)
        {
            return PixelOperator8();
        }

        return PixelOperator();
    }();

    if (!face.bReady)
    {
        return Font{};
    }

    return Font{ SizeFor(face, kDesignEm, true) };
}

Text::Size Text::Measure(Font font, const char* pszText)
{
    auto* const pSize = SizeOf(font);

    if (pSize == nullptr || pszText == nullptr)
    {
        return Size{ 0, 0 };
    }

    auto flWidth = 0.0f;

    for (const auto* p = reinterpret_cast<const unsigned char*>(pszText); *p != 0; p++)
    {
        flWidth += GetGlyph(*pSize, *p).flAdvance;

        if (p[1] != 0)
        {
            flWidth += Kerning(*pSize, p[0], p[1]);
        }
    }

    return Size{ static_cast<int>(std::ceil(flWidth)), pSize->nCellHeight };
}

void Text::Draw(Font font, unsigned char* pBits, int nPitch, int nWidth, int nHeight, int nX, int nY, const char* pszText, unsigned char nIndex)
{
    Rasterise(font, nWidth, nHeight, nX, nY, pszText, [&](int x, int y)
              {
                  pBits[y * nPitch + x] = nIndex;
              });
}

void Text::Draw(Font font, std::uint32_t* pBits, int nPitch, int nWidth, int nHeight, int nX, int nY, const char* pszText, std::uint32_t nColour)
{
    Rasterise(font, nWidth, nHeight, nX, nY, pszText, [&](int x, int y)
              {
                  *reinterpret_cast<std::uint32_t*>(reinterpret_cast<unsigned char*>(pBits) + y * nPitch + x * 4) = nColour;
              });
}
