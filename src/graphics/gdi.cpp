#include "common/compat.h"
#include "common/path.h"
#include "graphics/bitmapformat.h"
#include "graphics/gdi.h"
#include "graphics/mdm1.h"
#include "app/window.h"
#include "app/gameapp.h"
#include "globals.h"
#include "common/pathstring.h"
#include <filesystem>
#include <system_error>
#include "common/fileobj.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include "stb_image.h"
#include "graphics/text.h"

namespace
{

// Every bitmap a caller holds, as GDI owned the objects behind the original's handles.
auto s_bitmaps = std::vector<std::unique_ptr<GDI::Bitmap> >();

GDI::Bitmap* Hold(std::unique_ptr<GDI::Bitmap> pBitmap)
{
    auto* const pHandle = pBitmap.get();

    if (pHandle != nullptr)
    {
        s_bitmaps.push_back(std::move(pBitmap));
    }

    return pHandle;
}

// The DIB's pixels through its colour table, as CreateDIBitmap made a device bitmap of them for a
// true-colour display. Null for a depth the menus never use.
std::unique_ptr<GDI::Bitmap> FromDIB(const BitmapInfoHeader* pDib)
{
    const auto nWidth  = static_cast<int>(pDib->biWidth);
    const auto nHeight = std::abs(static_cast<int>(pDib->biHeight));
    const auto nBits   = static_cast<int>(pDib->biBitCount);

    if (nWidth <= 0 || nHeight <= 0 || pDib->biCompression != kBitmapRGB || (nBits != 1 && nBits != 4 && nBits != 8 && nBits != 24 && nBits != 32))
    {
        return nullptr;
    }

    const RgbQuad* const pColours = GDI::ColorTable(pDib);
    const auto           nColours = static_cast<int>(reinterpret_cast<const RgbQuad*>(GDI::PixelBits(pDib)) - pColours);
    const auto* const    pRows    = reinterpret_cast<const unsigned char*>(GDI::PixelBits(pDib));
    const auto           nStride  = (nWidth * nBits + 31) / 32 * 4;

    auto pBitmap = std::make_unique<GDI::Bitmap>(GDI::Bitmap{ nWidth, nHeight, std::vector<std::uint32_t>(static_cast<size_t>(nWidth) * nHeight) });

    for (int y = 0; y < nHeight; y++)
    {
        const auto nRow = [&]
        {
            // A positive height is a bottom-up DIB.
            if (pDib->biHeight > 0)
            {
                return nHeight - 1 - y;
            }

            return y;
        }();

        const auto* const pRow = pRows + static_cast<size_t>(nRow) * nStride;
        auto* const       pOut = &pBitmap->pixels[static_cast<size_t>(y) * nWidth];

        for (int x = 0; x < nWidth; x++)
        {
            if (nBits >= 24)
            {
                const auto* const p = pRow + x * (nBits / 8);

                pOut[x] = static_cast<std::uint32_t>(p[2] << 16 | p[1] << 8 | p[0]);

                continue;
            }

            const auto nPerByte = 8 / nBits;
            const auto nShift   = 8 - nBits * (x % nPerByte + 1);
            const auto nIndex   = (pRow[x / nPerByte] >> nShift) & ((1 << nBits) - 1);

            if (nIndex < nColours)
            {
                const auto& colour = pColours[nIndex];

                pOut[x] = static_cast<std::uint32_t>(colour.rgbRed << 16 | colour.rgbGreen << 8 | colour.rgbBlue);
            }
        }
    }

    return pBitmap;
}

} // namespace

// FUNCTION: HYPERX 0x00418cc0
void GDI::DrawBitmapToWindow(const char* pszResourceName)
{
    auto* const pBitmap = GDI::LoadBitmapWithPalette(pszResourceName, nullptr);

    if (pBitmap != nullptr)
    {
        GDI::Blit(0, 0, pBitmap->nWidth, pBitmap->nHeight, pBitmap, 0, 0);

        Window::PresentMenu();

        GDI::SafeDeleteBitmap(pBitmap);
    }
}

// A file starting with MDM1 goes to the decompressor.
// FUNCTION: HYPERX 0x0043fff0
Record<BitmapInfoHeader> GDI::ReadBitmapFile(const char* pszName)
{
    FileObj      file;
    unsigned int nMagic;

    if (file.Open(pszName, FileMode::kRead) == 0)
    {
        file.Close();

        return nullptr;
    }

    if (file.Read(&nMagic, sizeof(nMagic)) == sizeof(nMagic) && nMagic == kMdm1Magic)
    {
        auto pPacked = MDM1::Decompress(&file);

        file.CloseAndTest();
        file.Close();

        return GDI::ExpandRLE(std::move(pPacked));
    }

    file.SeekRelative(0, FileSeekOrigin::kStart);

    auto pDib = GDI::ReadDIB(&file);

    if (pDib == nullptr)
    {
        file.Close();

        return nullptr;
    }

    const int nImageBytes = pDib->biSizeImage;
    int       nColors     = pDib->biClrUsed;

    if (nColors == 0 && pDib->biBitCount <= 8)
    {
        nColors = 1 << pDib->biBitCount;
    }

    const int nTableBytes = pDib->biSize + nColors * sizeof(RgbQuad);
    const int nTotalBytes = nTableBytes + nImageBytes;

    // A negative image size failed the original's read.
    if (nImageBytes < 0)
    {
        file.CloseAndTest();
        file.Close();

        return nullptr;
    }

    // The header and its table move to a block with room for the pixels.
    auto pGrown = MakeRecord<BitmapInfoHeader>(nTotalBytes);

    memcpy(pGrown.get(), pDib.get(), nTableBytes);

    pDib = std::move(pGrown);

    int nGrownColors = pDib->biClrUsed;

    if (nGrownColors == 0 && pDib->biBitCount <= 8)
    {
        nGrownColors = 1 << pDib->biBitCount;
    }

    if (file.Read(ColorTable(pDib.get()) + nGrownColors, nImageBytes) != nImageBytes)
    {
        file.CloseAndTest();
        file.Close();

        return nullptr;
    }

    file.CloseAndTest();
    file.Close();

    return GDI::ExpandRLE(std::move(pDib));
}

// Every caller reads raw rows, so an RLE8 or RLE4 DIB is expanded to 8-bit kBitmapRGB in a new block.
Record<BitmapInfoHeader> GDI::ExpandRLE(Record<BitmapInfoHeader> pDib)
{
    if (pDib == nullptr || (pDib->biCompression != kBitmapRle8 && pDib->biCompression != kBitmapRle4))
    {
        return pDib;
    }

    const auto bFour   = pDib->biCompression == kBitmapRle4;
    const auto nWidth  = static_cast<int>(pDib->biWidth);
    const auto nHeight = static_cast<int>(pDib->biHeight);
    const auto nStride = (nWidth + 3) & ~3;

    const auto nTableBytes = static_cast<int>(GDI::PixelBits(pDib.get()) - reinterpret_cast<char*>(pDib.get()));
    const auto pIn         = reinterpret_cast<const unsigned char*>(GDI::PixelBits(pDib.get()));
    const auto nInBytes    = static_cast<int>(pDib->biSizeImage);

    if (nWidth <= 0 || nHeight <= 0)
    {
        return nullptr;
    }

    auto pOut = MakeRecord<BitmapInfoHeader>(nTableBytes + nStride * nHeight);

    memcpy(pOut.get(), pDib.get(), nTableBytes);

    auto* const pRows = reinterpret_cast<unsigned char*>(GDI::PixelBits(pOut.get()));

    const auto put = [&](int x, int y, int nPixel)
    {
        if (x < nWidth && y < nHeight)
        {
            pRows[y * nStride + x] = static_cast<unsigned char>(nPixel);
        }
    };

    const auto nibble = [&](int nByte, int i)
    {
        if (bFour == 0)
        {
            return nByte;
        }

        if ((i & 1) == 0)
        {
            return nByte >> 4;
        }

        return nByte & 0xf;
    };

    auto x  = 0;
    auto y  = 0;
    auto at = 0;

    while (at + 1 < nInBytes && y < nHeight)
    {
        const int nCount = pIn[at];
        const int nValue = pIn[at + 1];
        at += 2;

        if (nCount != 0)
        {
            for (int i = 0; i < nCount; i++)
            {
                put(x++, y, nibble(nValue, i));
            }
        }
        else if (nValue == 0)
        {
            x = 0;
            y++;
        }
        else if (nValue == 1)
        {
            break;
        }
        else if (nValue == 2)
        {
            if (at + 1 >= nInBytes)
            {
                break;
            }

            x += pIn[at];
            y += pIn[at + 1];
            at += 2;
        }
        else
        {
            const auto nRunBytes = [&]
            {
                if (bFour != 0)
                {
                    return (nValue + 1) / 2;
                }

                return nValue;
            }();

            if (at + nRunBytes > nInBytes)
            {
                break;
            }

            for (int i = 0; i < nValue; i++)
            {
                const auto nByte = [&]
                {
                    if (bFour != 0)
                    {
                        return static_cast<int>(pIn[at + i / 2]);
                    }

                    return static_cast<int>(pIn[at + i]);
                }();

                put(x++, y, nibble(nByte, i));
            }

            at += nRunBytes + (nRunBytes & 1);
        }
    }

    pOut->biBitCount    = 8;
    pOut->biCompression = kBitmapRGB;
    pOut->biSizeImage   = nStride * nHeight;
    pOut->biClrUsed     = (nTableBytes - pDib->biSize) / sizeof(RgbQuad);

    return pOut;
}

// A file's type word, "BM".
constexpr auto kBftBitmap = 0x4d42;

// A DIB's header, which its leading size word says is a Windows or an OS/2 one.
union DibHeader
{
    BitmapInfoHeader info;
    BitmapCoreHeader core;
};

// The file header is optional, and an OS/2 core header is widened in place.
// FUNCTION: HYPERX 0x00440200
Record<BitmapInfoHeader> GDI::ReadDIB(FileObj* pFile)
{
    BitmapFileHeader bmfh;
    DibHeader        header;
    RgbQuad          quad;
    int              nColors;

    if (pFile == nullptr)
    {
        return nullptr;
    }

    const long lStart = pFile->SeekRelative(0, FileSeekOrigin::kCurrent);

    if (pFile->Read(&bmfh, sizeof(bmfh)) != sizeof(bmfh))
    {
        pFile->CloseAndTest();

        return nullptr;
    }

    if (bmfh.bfType != kBftBitmap)
    {
        bmfh.bfOffBits = 0;

        pFile->SeekRelative(lStart, FileSeekOrigin::kStart);
    }

    if (pFile->Read(&header.info, sizeof(header.info)) != sizeof(header.info))
    {
        pFile->CloseAndTest();

        return nullptr;
    }

    const auto nHeaderSize = static_cast<unsigned int>(header.info.biSize);

    if (nHeaderSize == sizeof(BitmapCoreHeader))
    {
        const auto bmc = header.core;

        header.info.biSize          = sizeof(BitmapInfoHeader);
        header.info.biWidth         = bmc.bcWidth;
        header.info.biHeight        = bmc.bcHeight;
        header.info.biPlanes        = bmc.bcPlanes;
        header.info.biBitCount      = bmc.bcBitCount;
        header.info.biCompression   = kBitmapRGB;
        header.info.biSizeImage     = 0;
        header.info.biXPelsPerMeter = 0;
        header.info.biYPelsPerMeter = 0;
        header.info.biClrUsed       = 0;
        header.info.biClrImportant  = 0;

        pFile->SeekRelative(static_cast<long>(sizeof(BitmapCoreHeader)) - static_cast<long>(sizeof(BitmapInfoHeader)), FileSeekOrigin::kCurrent);
    }

    if (header.info.biClrUsed == 0 && header.info.biBitCount <= 8)
    {
        nColors = 1 << header.info.biBitCount;
    }
    else
    {
        nColors = header.info.biClrUsed;
    }

    if (header.info.biSizeImage == 0)
    {
        header.info.biSizeImage = (((static_cast<unsigned int>(header.info.biBitCount * header.info.biWidth) + 31) >> 3) & ~3) * header.info.biHeight;
    }

    if (header.info.biClrUsed == 0 && header.info.biBitCount <= 8)
    {
        header.info.biClrUsed = 1 << header.info.biBitCount;
    }

    auto pDib = MakeRecord<BitmapInfoHeader>(header.info.biSize + nColors * sizeof(RgbQuad));

    *pDib = header.info;

    auto* const pColors = GDI::ColorTable(pDib.get());

    if (nColors != 0)
    {
        if (nHeaderSize == sizeof(BitmapCoreHeader))
        {
            const int nTableBytes = nColors * sizeof(RgbTriple);

            if (pFile->Read(pColors, nTableBytes) != nTableBytes)
            {
                pFile->CloseAndTest();

                return nullptr;
            }

            auto* const pTriples = reinterpret_cast<RgbTriple*>(pColors);

            for (int i = nColors - 1; i >= 0; i--)
            {
                quad.rgbReserved = 0;
                quad.rgbRed      = pTriples[i].rgbtRed;
                quad.rgbBlue     = pTriples[i].rgbtBlue;
                quad.rgbGreen    = pTriples[i].rgbtGreen;

                pColors[i] = quad;
            }
        }
        else
        {
            const int nTableBytes = nColors * sizeof(RgbQuad);

            if (pFile->Read(pColors, nTableBytes) != nTableBytes)
            {
                pFile->CloseAndTest();

                return nullptr;
            }
        }
    }

    if (bmfh.bfOffBits != 0)
    {
        pFile->SeekRelative(lStart + bmfh.bfOffBits, FileSeekOrigin::kStart);
    }

    return pDib;
}

// FUNCTION: HYPERX 0x0048f330
GDI::Bitmap* GDI::LoadBitmapHandle(const char* pszName)
{
    return GDI::LoadBitmapWithPalette(pszName, nullptr);
}

// FUNCTION: HYPERX 0x0048f340
GDI::Bitmap* GDI::LoadBitmapAsBackground(const char* pszName, int* pbPaletted)
{
    return GDI::LoadBitmapWithPalette(pszName, pbPaletted);
}

// Every caller draws onto the menu canvas, in the System font's sixteen-pixel cell, which the
// original never replaced with a font of its own.
// FUNCTION: HYPERX 0x0048f3b0
void GDI::DrawTransparentText(std::uint32_t color, int x, int y, const char* pszText, int, int)
{
    constexpr auto kSystemFontHeight = 16;

    const auto canvas = Window::MenuPixels();
    const auto nRGB   = (color & 0xff) << 16 | (color & 0xff00) | (color >> 16 & 0xff);

    Text::Draw(Text::Find(kSystemFontHeight), canvas.pBits, canvas.nWidth * 4, canvas.nWidth, canvas.nHeight, x, y, pszText, nRGB);
}

// FUNCTION: HYPERX 0x0048f410
void GDI::SafeDeleteBitmap(Bitmap* pBitmap)
{
    std::erase_if(
        s_bitmaps,
        [&](const std::unique_ptr<Bitmap>& pHeld)
        {
            return pHeld.get() == pBitmap;
        });
}

// A null bitmap leaves the size alone, as GetObject failing did.
// FUNCTION: HYPERX 0x0048f430
void GDI::GetBitmapDimensions(const Bitmap* pBitmap, BitmapSize* pSize)
{
    if (pBitmap != nullptr)
    {
        pSize->nWidth  = pBitmap->nWidth;
        pSize->nHeight = pBitmap->nHeight;
    }
}

// Copies onto the menu canvas, clipped to both the canvas and the bitmap.
// FUNCTION: HYPERX 0x0048f4b0
void GDI::Blit(int x, int y, int cx, int cy, const Bitmap* pBitmap, int nSourceX, int nSourceY)
{
    if (pBitmap == nullptr)
    {
        return;
    }

    const auto canvas = Window::MenuPixels();

    for (int nRow = 0; nRow < cy; nRow++)
    {
        const auto nFromY = nSourceY + nRow;
        const auto nToY   = y + nRow;

        if (nFromY < 0 || nFromY >= pBitmap->nHeight || nToY < 0 || nToY >= canvas.nHeight)
        {
            continue;
        }

        for (int nColumn = 0; nColumn < cx; nColumn++)
        {
            const auto nFromX = nSourceX + nColumn;
            const auto nToX   = x + nColumn;

            if (nFromX >= 0 && nFromX < pBitmap->nWidth && nToX >= 0 && nToX < canvas.nWidth)
            {
                canvas.pBits[nToY * canvas.nWidth + nToX] = pBitmap->pixels[static_cast<size_t>(nFromY * pBitmap->nWidth + nFromX)];
            }
        }
    }
}

// A JPEG has no palette, so the flag is left alone.
// FUNCTION: HYPERX 0x0048f540
GDI::Bitmap* GDI::LoadBitmapWithPalette(const char* pszResourceName, int* pbPaletted)
{
    PathString strPath(kMaxPathLength);

    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, pszResourceName, "bmp");

    if (std::error_code error; !std::filesystem::exists(Path::Native(strPath.m_str.m_pchData), error))
    {
        // STRING: HYPERX 0x004c8960
        strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, pszResourceName, "jpg");

        if (std::error_code jpgError; !std::filesystem::exists(Path::Native(strPath.m_str.m_pchData), jpgError))
        {
            return nullptr;
        }

        return GDI::LoadJPEGImage(strPath.m_str.m_pchData);
    }

    const auto pDib = GDI::ReadBitmapFile(strPath.m_str.m_pchData);

    if (pDib == nullptr)
    {
        return nullptr;
    }

    if (pbPaletted != nullptr && pDib->biBitCount <= 8)
    {
        *pbPaletted = 1;
    }

    auto* const pBitmap = Hold(FromDIB(pDib.get()));

    if (pBitmap == nullptr)
    {
        // STRING: HYPERX 0x004bc42c
        g_gameApp.Error("Problem Creating DIBitmap %s, check palette!", pszResourceName);
    }

    return pBitmap;
}

// FUNCTION: HYPERX 0x0048f7d0
GDI::Bitmap* GDI::LoadJPEGImage(const char* pszPath)
{
    auto nWidth    = 0;
    auto nHeight   = 0;
    auto nChannels = 0;

    stbi_set_flip_vertically_on_load_thread(0);

    auto* const pPixels = stbi_load(Path::Native(pszPath).c_str(), &nWidth, &nHeight, &nChannels, 3);

    if (pPixels == nullptr)
    {
        return nullptr;
    }

    auto* const pBitmap = Hold(std::make_unique<Bitmap>(Bitmap{ nWidth, nHeight, std::vector<std::uint32_t>(static_cast<size_t>(nWidth) * nHeight) }));

    for (size_t i = 0; i < pBitmap->pixels.size(); i++)
    {
        const stbi_uc* const p = pPixels + i * 3;

        pBitmap->pixels[i] = static_cast<std::uint32_t>(p[0] << 16 | p[1] << 8 | p[2]);
    }

    stbi_image_free(pPixels);

    return pBitmap;
}
