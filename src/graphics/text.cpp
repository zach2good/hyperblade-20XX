#include "graphics/text.h"
#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <string>
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

struct FontSize
{
    float                  flScale     = 0.0f;
    int                    nAscent     = 0;
    int                    nCellHeight = 0;
    std::array<Glyph, 256> glyphs;
    std::array<bool, 256>  abBuilt = {};
};

struct Face
{
    std::vector<unsigned char> file;
    stbtt_fontinfo             info   = {};
    bool                       bReady = false;
};

Face& LoadFace()
{
    static auto s_face = []
    {
        auto face = Face{};

        // TODO: Fall back to a bundled open font where Windows' is missing.
        const auto path = [&]
        {
            const auto* const pszWindows = SDL_getenv("WINDIR");

            if (pszWindows == nullptr)
            {
                return std::string("C:\\Windows\\Fonts\\micross.ttf");
            }

            return std::string(pszWindows) + "\\Fonts\\micross.ttf";
        }();

        auto nSize = (size_t)0;

        void* const pData = SDL_LoadFile(path.c_str(), &nSize);

        if (pData == nullptr)
        {
            SDL_Log("Text: %s did not load: %s", path.c_str(), SDL_GetError());

            return face;
        }

        face.file.assign((const unsigned char*)pData, (const unsigned char*)pData + nSize);

        SDL_free(pData);

        face.bReady = stbtt_InitFont(&face.info, face.file.data(), stbtt_GetFontOffsetForIndex(face.file.data(), 0)) != 0;

        return face;
    }();

    return s_face;
}

std::map<int, std::unique_ptr<FontSize> > s_sizes;

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

    auto& face = LoadFace();

    auto nAdvance = 0;
    auto nBearing = 0;

    stbtt_GetCodepointHMetrics(&face.info, nChar, &nAdvance, &nBearing);

    glyph.flAdvance = nAdvance * size.flScale;

    auto nX0 = 0;
    auto nY0 = 0;
    auto nX1 = 0;
    auto nY1 = 0;

    stbtt_GetCodepointBitmapBox(&face.info, nChar, size.flScale, size.flScale, &nX0, &nY0, &nX1, &nY1);

    glyph.nWidth   = nX1 - nX0;
    glyph.nHeight  = nY1 - nY0;
    glyph.nOffsetX = nX0;
    glyph.nOffsetY = nY0;

    if (glyph.nWidth <= 0 || glyph.nHeight <= 0)
    {
        return glyph;
    }

    glyph.mask.resize((size_t)(glyph.nWidth * glyph.nHeight));

    stbtt_MakeCodepointBitmap(&face.info, glyph.mask.data(), glyph.nWidth, glyph.nHeight, glyph.nWidth, size.flScale, size.flScale, nChar);

    for (auto& nCoverage : glyph.mask)
    {
        nCoverage = (unsigned char)(nCoverage >= 128);
    }

    return glyph;
}

// Calls plot(x, y) for every pixel the text sets, clipped to the target.
template <class Plot>
void Rasterise(Text::Font font, int nWidth, int nHeight, int nX, int nY, const char* pszText, Plot plot)
{
    FontSize* const pSize = SizeOf(font);

    if (pSize == nullptr || pszText == nullptr)
    {
        return;
    }

    const auto nBaseline = nY + pSize->nAscent;

    auto flPen = (float)nX;

    for (const auto* p = (const unsigned char*)pszText; *p != 0; p++)
    {
        const auto& glyph = GetGlyph(*pSize, *p);

        const auto nLeft = (int)std::floor(flPen) + glyph.nOffsetX;
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

                if (glyph.mask[(size_t)(y * glyph.nWidth + x)] != 0 && nColumn >= 0 && nColumn < nWidth)
                {
                    plot(nColumn, nRow);
                }
            }
        }

        flPen += glyph.flAdvance;

        if (p[1] != 0)
        {
            flPen += stbtt_GetCodepointKernAdvance(&LoadFace().info, p[0], p[1]) * pSize->flScale;
        }
    }
}

} // namespace

Text::Font Text::Find(int nHeight)
{
    auto& face = LoadFace();

    if (!face.bReady || nHeight <= 0)
    {
        return Font{};
    }

    auto& pSize = s_sizes[nHeight];

    if (pSize == nullptr)
    {
        pSize = std::make_unique<FontSize>();

        pSize->flScale = stbtt_ScaleForPixelHeight(&face.info, (float)nHeight);

        auto nAscent  = 0;
        auto nDescent = 0;
        auto nLineGap = 0;

        stbtt_GetFontVMetrics(&face.info, &nAscent, &nDescent, &nLineGap);

        pSize->nAscent     = (int)std::lround(nAscent * pSize->flScale);
        pSize->nCellHeight = nHeight;
    }

    return Font{ pSize.get() };
}

Text::Size Text::Measure(Font font, const char* pszText)
{
    FontSize* const pSize = SizeOf(font);

    if (pSize == nullptr || pszText == nullptr)
    {
        return Size{ 0, 0 };
    }

    auto flWidth = 0.0f;

    for (const auto* p = (const unsigned char*)pszText; *p != 0; p++)
    {
        flWidth += GetGlyph(*pSize, *p).flAdvance;

        if (p[1] != 0)
        {
            flWidth += stbtt_GetCodepointKernAdvance(&LoadFace().info, p[0], p[1]) * pSize->flScale;
        }
    }

    return Size{ (int)std::ceil(flWidth), pSize->nCellHeight };
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
                  *(std::uint32_t*)((unsigned char*)pBits + y * nPitch + x * 4) = nColour;
              });
}
