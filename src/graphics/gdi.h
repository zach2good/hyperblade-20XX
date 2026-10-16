#ifndef HYPERBLADE_GDI_H
#define HYPERBLADE_GDI_H
#include "common/compat.h"

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

HBITMAP                   LoadBitmapHandle(const char* pszName);
HBITMAP                   LoadBitmapAsBackground(const char* pszName, HPALETTE* phPalette);
HBITMAP                   LoadBitmapWithPalette(LPCSTR pszResourceName, HPALETTE* phPalette);
void                      SafeDeleteBitmap(HBITMAP hBitmap);
void                      GetBitmapDimensions(HBITMAP hBitmap, BITMAP* pBitmap);
BITMAPINFOHEADER* __cdecl ReadBitmapFile(const char* pszName);
BITMAPINFOHEADER*         ReadDIB(FileObj* pFile);
BITMAPINFOHEADER*         ExpandRLE(BITMAPINFOHEADER* pDib);
HPALETTE                  CreatePaletteFromDIB(BITMAPINFO* pbmi, int* pcColors);
HPALETTE                  SelectSplashPalette(HDC hdc, HPALETTE hPalette, BOOL bForceBackground);
UINT                      RealizePalette(HDC hdc);
HBITMAP                   LoadJPEGImage(const char* pszPath);
void                      QueryDisplayCaps(void);
BOOL                      EndPaint(const PAINTSTRUCT* pPaint);
BOOL                      DeleteObject(HGDIOBJ hObject);
int                       BlitBitmapToDC(HDC hdc, int x, int y, int cx, int cy, HBITMAP hBitmap, HPALETTE hPalette, int x1, int y1, DWORD rop);
void                      DrawTransparentText(HDC hdc, COLORREF color, int x, int y, const char* pszText, int cchText, int nUnused);
HDC                       BeginPaint(LPPAINTSTRUCT pPaint);
HPALETTE                  DrawBitmapToWindow(HWND hWnd, LPCSTR pszResourceName);

} // namespace GDI

#endif
