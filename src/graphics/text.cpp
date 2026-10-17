#include "graphics/text.h"
#include <SDL3/SDL.h>
#include <algorithm>
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
    bool                   bBitmap     = false; // Drawn from a Windows bitmap font, with no kerning.
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

// TODO: Fall back to bundled open fonts where Windows' are missing.
std::vector<unsigned char> LoadFontFile(const char* pszName)
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

    auto nSize = (size_t)0;

    void* const pData = SDL_LoadFile(path.c_str(), &nSize);

    if (pData == nullptr)
    {
        SDL_Log("Text: %s did not load: %s", path.c_str(), SDL_GetError());

        return {};
    }

    auto file = std::vector<unsigned char>((const unsigned char*)pData, (const unsigned char*)pData + nSize);

    SDL_free(pData);

    return file;
}

Face& LoadFace()
{
    static auto s_face = []
    {
        auto face = Face{};

        face.file = LoadFontFile("micross.ttf");

        if (face.file.empty())
        {
            return face;
        }

        face.bReady = stbtt_InitFont(&face.info, face.file.data(), stbtt_GetFontOffsetForIndex(face.file.data(), 0)) != 0;

        return face;
    }();

    return s_face;
}

std::map<int, std::unique_ptr<FontSize> > s_sizes;

// Little-endian reads that mark the file bad instead of reading past its end.
struct FontBytes
{
    const std::vector<unsigned char>& bytes;
    bool                              bValid = true;

    unsigned int U8(size_t nAt)
    {
        if (nAt >= bytes.size())
        {
            bValid = false;

            return 0;
        }

        return bytes[nAt];
    }

    unsigned int U16(size_t nAt)
    {
        return U8(nAt) | U8(nAt + 1) << 8;
    }

    unsigned int U32(size_t nAt)
    {
        return U16(nAt) | U16(nAt + 2) << 16;
    }
};

constexpr auto RT_FONT_ID   = 0x8008u;
constexpr auto MAX_FONT_RES = 64u;

// One size of a Windows .FNT resource. Each glyph is stored as strips eight pixels wide, a byte a
// row, the highest bit leftmost.
std::unique_ptr<FontSize> ReadFnt(FontBytes& in, size_t nAt)
{
    const auto nVersion = in.U16(nAt);

    auto size = std::make_unique<FontSize>();

    size->bBitmap     = true;
    size->nAscent     = (int)in.U16(nAt + 74);
    size->nCellHeight = (int)in.U16(nAt + 88);

    const auto nFirst = in.U8(nAt + 95);
    const auto nLast  = in.U8(nAt + 96);

    // Version 3 widens the header and each glyph's offset.
    const auto bWide = nVersion >= 0x300;

    const auto nTable = [&]
    {
        if (bWide)
        {
            return nAt + 148;
        }

        return nAt + 118;
    }();

    const auto nEntrySize = [&]
    {
        if (bWide)
        {
            return 6u;
        }

        return 4u;
    }();

    for (auto nChar = nFirst; nChar <= nLast && in.bValid; nChar++)
    {
        const auto nEntry = nTable + (nChar - nFirst) * nEntrySize;
        const auto nWidth = (int)in.U16(nEntry);

        const auto nBits = [&]
        {
            if (bWide)
            {
                return nAt + in.U32(nEntry + 2);
            }

            return nAt + in.U16(nEntry + 2);
        }();

        auto& glyph = size->glyphs[nChar];

        glyph.nWidth    = nWidth;
        glyph.nHeight   = size->nCellHeight;
        glyph.nOffsetY  = -size->nAscent;
        glyph.flAdvance = (float)nWidth;
        glyph.mask.assign((size_t)(nWidth * size->nCellHeight), 0);

        for (int y = 0; y < size->nCellHeight; y++)
        {
            for (int x = 0; x < nWidth; x++)
            {
                const auto nByte = in.U8(nBits + (size_t)(x / 8 * size->nCellHeight + y));

                glyph.mask[(size_t)(y * nWidth + x)] = (unsigned char)(nByte >> (7 - x % 8) & 1);
            }
        }
    }

    size->abBuilt.fill(true);

    return size;
}

// The sizes of the bitmap MS Sans Serif, smallest first; empty when its file is missing or bad.
const std::vector<std::unique_ptr<FontSize> >& LoadSansSerif()
{
    static auto s_bitmapSizes = []
    {
        auto sizes = std::vector<std::unique_ptr<FontSize> >();

        const auto file = LoadFontFile("sserife.fon");

        auto in = FontBytes{ file };

        const auto nNE = (size_t)in.U32(0x3c);

        if (file.empty() || in.U16(nNE) != 0x454e)
        {
            return sizes;
        }

        const auto nTable = nNE + in.U16(nNE + 0x24);
        const auto nShift = in.U16(nTable);

        auto nAt = nTable + 2;

        for (unsigned int nType = 0; nType < MAX_FONT_RES && in.bValid; nType++)
        {
            const auto nTypeID = in.U16(nAt);
            const auto nCount  = (std::min)(in.U16(nAt + 2), MAX_FONT_RES);

            if (nTypeID == 0)
            {
                break;
            }

            for (unsigned int n = 0; n < nCount && in.bValid; n++)
            {
                const auto nEntry = nAt + 8 + n * 12;

                if (nTypeID == RT_FONT_ID)
                {
                    sizes.push_back(ReadFnt(in, (size_t)in.U16(nEntry) << nShift));
                }
            }

            nAt += 8 + nCount * 12;
        }

        if (!in.bValid)
        {
            sizes.clear();
        }

        std::sort(sizes.begin(), sizes.end(), [](const auto& a, const auto& b)
                  {
                      return a->nCellHeight < b->nCellHeight;
                  });

        return sizes;
    }();

    return s_bitmapSizes;
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

        if (p[1] != 0 && !pSize->bBitmap)
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

// As GDI matched a raster font: the largest size no taller than asked, or else the smallest.
Text::Font Text::FindSansSerif(int nHeight)
{
    const auto& sizes = LoadSansSerif();

    if (sizes.empty())
    {
        return Find(nHeight);
    }

    auto* pChosen = sizes.front().get();

    for (const auto& pSize : sizes)
    {
        if (pSize->nCellHeight <= nHeight)
        {
            pChosen = pSize.get();
        }
    }

    return Font{ pChosen };
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

        if (p[1] != 0 && !pSize->bBitmap)
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
