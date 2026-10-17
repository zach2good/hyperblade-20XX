#ifndef HYPERBLADE_GDI_H
#define HYPERBLADE_GDI_H
#include "common/compat.h"
#include <cstdint>
#include <vector>

class FileObj;

namespace GDI
{

// The colour table follows the header, which records its own size.
inline RGBQUAD* ColorTable(const BITMAPINFOHEADER* pHeader)
{
    return (RGBQUAD*)((const char*)pHeader + pHeader->biSize);
}

// A zero biClrUsed means the full table for the bit depth.
inline char* PixelBits(const BITMAPINFOHEADER* pHeader)
{
    const auto nColours = [&]
    {
        if (pHeader->biClrUsed != 0)
        {
            return (int)pHeader->biClrUsed;
        }

        if (pHeader->biBitCount <= 8)
        {
            return 1 << pHeader->biBitCount;
        }

        return 0;
    }();

    return (char*)(ColorTable(pHeader) + nColours);
}

// A picture for the menu canvas, 0x00RRGGBB, top row first.
struct Bitmap
{
    int                        nWidth;
    int                        nHeight;
    std::vector<std::uint32_t> pixels;
};

// What the screens read of GetObject's BITMAP.
struct BitmapSize
{
    int bmWidth;
    int bmHeight;
};

Bitmap*           LoadBitmapHandle(const char* pszName);
Bitmap*           LoadBitmapAsBackground(const char* pszName, int* pbPaletted);
Bitmap*           LoadBitmapWithPalette(const char* pszResourceName, int* pbPaletted);
void              SafeDeleteBitmap(Bitmap* pBitmap);
void              GetBitmapDimensions(const Bitmap* pBitmap, BitmapSize* pSize);
BITMAPINFOHEADER* ReadBitmapFile(const char* pszName);
BITMAPINFOHEADER* ReadDIB(FileObj* pFile);
BITMAPINFOHEADER* ExpandRLE(BITMAPINFOHEADER* pDib);
Bitmap*           LoadJPEGImage(const char* pszPath);
void              Blit(int x, int y, int cx, int cy, const Bitmap* pBitmap, int nSourceX, int nSourceY);
void              DrawTransparentText(COLORREF color, int x, int y, const char* pszText, int cchText, int nUnused);
void              DrawBitmapToWindow(const char* pszResourceName);

} // namespace GDI

#endif
