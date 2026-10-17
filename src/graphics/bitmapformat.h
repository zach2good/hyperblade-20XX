// The Windows bitmap and palette layouts the game's files are stored in.

#ifndef HYPERBLADE_BITMAPFORMAT_H
#define HYPERBLADE_BITMAPFORMAT_H

#include <cstdint>

struct PaletteEntry
{
    std::uint8_t peRed;
    std::uint8_t peGreen;
    std::uint8_t peBlue;
    std::uint8_t peFlags;
};

constexpr std::uint8_t PALETTE_RESERVED   = 0x01;
constexpr std::uint8_t PALETTE_NOCOLLAPSE = 0x04;

struct RgbQuad
{
    std::uint8_t rgbBlue;
    std::uint8_t rgbGreen;
    std::uint8_t rgbRed;
    std::uint8_t rgbReserved;
};

struct RgbTriple
{
    std::uint8_t rgbtBlue;
    std::uint8_t rgbtGreen;
    std::uint8_t rgbtRed;
};

#pragma pack(push, 2)

struct BitmapFileHeader
{
    std::uint16_t bfType;
    std::uint32_t bfSize;
    std::uint16_t bfReserved1;
    std::uint16_t bfReserved2;
    std::uint32_t bfOffBits;
};

#pragma pack(pop)

struct BitmapInfoHeader
{
    std::uint32_t biSize;
    std::int32_t  biWidth;
    std::int32_t  biHeight;
    std::uint16_t biPlanes;
    std::uint16_t biBitCount;
    std::uint32_t biCompression;
    std::uint32_t biSizeImage;
    std::int32_t  biXPelsPerMeter;
    std::int32_t  biYPelsPerMeter;
    std::uint32_t biClrUsed;
    std::uint32_t biClrImportant;
};

// The OS/2 header an old bitmap may carry instead.
struct BitmapCoreHeader
{
    std::uint32_t bcSize;
    std::uint16_t bcWidth;
    std::uint16_t bcHeight;
    std::uint16_t bcPlanes;
    std::uint16_t bcBitCount;
};

constexpr std::uint32_t BITMAP_RGB  = 0;
constexpr std::uint32_t BITMAP_RLE8 = 1;
constexpr std::uint32_t BITMAP_RLE4 = 2;

static_assert(sizeof(PaletteEntry) == 4);
static_assert(sizeof(RgbQuad) == 4);
static_assert(sizeof(RgbTriple) == 3);
static_assert(sizeof(BitmapFileHeader) == 14);
static_assert(sizeof(BitmapInfoHeader) == 40);
static_assert(sizeof(BitmapCoreHeader) == 12);

#endif
