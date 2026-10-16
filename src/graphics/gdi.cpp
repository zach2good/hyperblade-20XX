#include "common/compat.h"
#include <stdlib.h>
#include "graphics/gdi.h"
#include "app/window.h"
#include "app/gameapp.h"
#include "globals.h"
#include "common/pathstring.h"
#include <io.h>
#include "common/fileobj.h"
#include <string.h>
#include <stdio.h>
#include "stb_image.h"

// GLOBAL: HYPERX 0x004c2ca4
extern int g_nDisplayBitsPerPixel;

// GLOBAL: HYPERX 0x004cee88
extern int g_nDisplayHorzRes;

// GLOBAL: HYPERX 0x004cee8c
extern int g_nDisplayVertRes;

// The loader's palette is returned whether or not anything was drawn.
// FUNCTION: HYPERX 0x00418cc0
HPALETTE GDI::DrawBitmapToWindow(HWND hWnd, LPCSTR pszResourceName)
{
    HPALETTE hPalette = 0;
    BITMAP   bm;
    HDC      hdcMem;
    HGDIOBJ  hOld;

    const HBITMAP hbm = GDI::LoadBitmapWithPalette(pszResourceName, &hPalette);

    if (hbm != 0)
    {
        if (GetObjectA(hbm, sizeof(bm), &bm) != 0)
        {
            const HDC hdc = Window::MenuDC();

            hdcMem = CreateCompatibleDC(hdc);

            if (hdcMem != 0)
            {
                if (bm.bmBitsPixel == 8)
                {
                    SelectPalette(hdcMem, hPalette, 0);
                    ::RealizePalette(hdcMem);
                }

                hOld = SelectObject(hdcMem, hbm);

                BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

                SelectObject(hdcMem, hOld);
                DeleteDC(hdcMem);
            }

            Window::PresentMenu(hWnd);
        }

        ::DeleteObject(hbm);
    }

    return hPalette;
}

// A file starting with MDM1 goes to the decompressor.
// FUNCTION: HYPERX 0x0043fff0
BITMAPINFOHEADER* __cdecl GDI::ReadBitmapFile(const char* pszName)
{
    FileObj           file;
    unsigned int      nMagic;
    BITMAPINFOHEADER* pDib;

    file.m_fd = 0;

    if (file.Open(pszName, FILE_MODE_READ) == 0)
    {
        file.Close();

        return 0;
    }

    if (file.Read(&nMagic, sizeof(nMagic)) == sizeof(nMagic) && nMagic == MDM1_MAGIC)
    {
        pDib = (BITMAPINFOHEADER*)MDM1::Decompress(&file);

        file.CloseAndTest();
        file.Close();

        return GDI::ExpandRLE(pDib);
    }

    file.SeekRelative(0, FILE_SEEK_START);

    pDib = GDI::ReadDIB(&file);

    if (pDib == 0)
    {
        file.Close();

        return 0;
    }

    const int nImageBytes = pDib->biSizeImage;
    int       nColors     = pDib->biClrUsed;

    if (nColors == 0 && pDib->biBitCount <= 8)
    {
        nColors = 1 << pDib->biBitCount;
    }

    const int nTotalBytes = pDib->biSize + nColors * sizeof(RGBQUAD) + nImageBytes;

    BITMAPINFOHEADER* pShrunk = (BITMAPINFOHEADER*)realloc(pDib, nTotalBytes);

    if (pShrunk == 0)
    {
        free(pDib);

        pDib = 0;
    }
    else
    {
        pDib = pShrunk;

        int nShrunkColors = pShrunk->biClrUsed;

        if (nShrunkColors == 0 && pShrunk->biBitCount <= 8)
        {
            nShrunkColors = 1 << pShrunk->biBitCount;
        }

        if (file.Read(ColorTable(pShrunk) + nShrunkColors, nImageBytes) != nImageBytes)
        {
            file.CloseAndTest();
            file.Close();

            return 0;
        }
    }

    file.CloseAndTest();
    file.Close();

    return GDI::ExpandRLE(pDib);
}

// Every caller reads raw rows, so an RLE8 or RLE4 DIB is expanded to 8-bit BI_RGB in a new block.
BITMAPINFOHEADER* GDI::ExpandRLE(BITMAPINFOHEADER* pDib)
{
    if (pDib == 0 || (pDib->biCompression != BI_RLE8 && pDib->biCompression != BI_RLE4))
    {
        return pDib;
    }

    const auto bFour   = pDib->biCompression == BI_RLE4;
    const auto nWidth  = (int)pDib->biWidth;
    const auto nHeight = (int)pDib->biHeight;
    const auto nStride = (nWidth + 3) & ~3;

    const auto nTableBytes = (int)(GDI::PixelBits(pDib) - (char*)pDib);
    const auto pIn         = (const unsigned char*)GDI::PixelBits(pDib);
    const auto nInBytes    = (int)pDib->biSizeImage;

    auto* const pOut = (BITMAPINFOHEADER*)calloc(1, nTableBytes + nStride * nHeight);

    if (pOut == 0 || nWidth <= 0 || nHeight <= 0)
    {
        free(pOut);
        free(pDib);

        return 0;
    }

    memcpy(pOut, pDib, nTableBytes);

    auto* const pRows = (unsigned char*)GDI::PixelBits(pOut);

    const auto put = [&](int x, int y, int nPixel)
    {
        if (x < nWidth && y < nHeight)
        {
            pRows[y * nStride + x] = (unsigned char)nPixel;
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
                        return (int)pIn[at + i / 2];
                    }

                    return (int)pIn[at + i];
                }();

                put(x++, y, nibble(nByte, i));
            }

            at += nRunBytes + (nRunBytes & 1);
        }
    }

    pOut->biBitCount    = 8;
    pOut->biCompression = BI_RGB;
    pOut->biSizeImage   = nStride * nHeight;
    pOut->biClrUsed     = (nTableBytes - pDib->biSize) / sizeof(RGBQUAD);

    free(pDib);

    return pOut;
}

// A file's type word, "BM".
#define BFT_BITMAP 0x4d42

// A DIB's header, which its leading size word says is a Windows or an OS/2 one.
union DibHeader
{
    BITMAPINFOHEADER info;
    BITMAPCOREHEADER core;
};

// The file header is optional, and an OS/2 core header is widened in place.
// FUNCTION: HYPERX 0x00440200
BITMAPINFOHEADER* GDI::ReadDIB(FileObj* pFile)
{
    BITMAPFILEHEADER bmfh;
    DibHeader        header;
    RGBQUAD          quad;
    int              nColors;

    if (pFile == 0)
    {
        return 0;
    }

    const long lStart = pFile->SeekRelative(0, FILE_SEEK_CURRENT);

    if (pFile->Read(&bmfh, sizeof(bmfh)) != sizeof(bmfh))
    {
        pFile->CloseAndTest();

        return 0;
    }

    if (bmfh.bfType != BFT_BITMAP)
    {
        bmfh.bfOffBits = 0;

        pFile->SeekRelative(lStart, FILE_SEEK_START);
    }

    if (pFile->Read(&header.info, sizeof(header.info)) != sizeof(header.info))
    {
        pFile->CloseAndTest();

        return 0;
    }

    const DWORD dwHeaderSize = header.info.biSize;

    if (dwHeaderSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER bmc = header.core;

        header.info.biSize          = sizeof(BITMAPINFOHEADER);
        header.info.biWidth         = bmc.bcWidth;
        header.info.biHeight        = bmc.bcHeight;
        header.info.biPlanes        = bmc.bcPlanes;
        header.info.biBitCount      = bmc.bcBitCount;
        header.info.biCompression   = BI_RGB;
        header.info.biSizeImage     = 0;
        header.info.biXPelsPerMeter = 0;
        header.info.biYPelsPerMeter = 0;
        header.info.biClrUsed       = 0;
        header.info.biClrImportant  = 0;

        pFile->SeekRelative((long)sizeof(BITMAPCOREHEADER) - (long)sizeof(BITMAPINFOHEADER), FILE_SEEK_CURRENT);
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
        header.info.biSizeImage = ((((DWORD)(header.info.biBitCount * header.info.biWidth) + 31) >> 3) & ~3) * header.info.biHeight;
    }

    if (header.info.biClrUsed == 0 && header.info.biBitCount <= 8)
    {
        header.info.biClrUsed = 1 << header.info.biBitCount;
    }

    BITMAPINFOHEADER* const pDib = (BITMAPINFOHEADER*)malloc(header.info.biSize + nColors * sizeof(RGBQUAD));

    if (pDib == 0)
    {
        return 0;
    }

    *pDib = header.info;

    RGBQUAD* const pColors = GDI::ColorTable(pDib);

    if (nColors != 0)
    {
        if (dwHeaderSize == sizeof(BITMAPCOREHEADER))
        {
            const int nTableBytes = nColors * sizeof(RGBTRIPLE);

            if (pFile->Read(pColors, nTableBytes) != nTableBytes)
            {
                pFile->CloseAndTest();

                return 0;
            }

            RGBTRIPLE* const pTriples = (RGBTRIPLE*)pColors;

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
            const int nTableBytes = nColors * sizeof(RGBQUAD);

            if (pFile->Read(pColors, nTableBytes) != nTableBytes)
            {
                pFile->CloseAndTest();

                return 0;
            }
        }
    }

    if (bmfh.bfOffBits != 0)
    {
        pFile->SeekRelative(lStart + bmfh.bfOffBits, FILE_SEEK_START);
    }

    return pDib;
}

// FUNCTION: HYPERX 0x0044b810
void GDI::QueryDisplayCaps(void)
{
    const HDC hdc = GetWindowDC(g_hWndGame);

    g_nDisplayBitsPerPixel = GetDeviceCaps(hdc, BITSPIXEL);
    g_nDisplayHorzRes      = GetDeviceCaps(hdc, HORZRES);
    g_nDisplayVertRes      = GetDeviceCaps(hdc, VERTRES);
}

// FUNCTION: HYPERX 0x0048f330
HBITMAP GDI::LoadBitmapHandle(const char* pszName)
{
    return GDI::LoadBitmapWithPalette(pszName, 0);
}

// FUNCTION: HYPERX 0x0048f340
HBITMAP GDI::LoadBitmapAsBackground(const char* pszName, HPALETTE* phPalette)
{
    return GDI::LoadBitmapWithPalette(pszName, phPalette);
}

// FUNCTION: HYPERX 0x0048f360
HDC GDI::BeginPaint(LPPAINTSTRUCT pPaint)
{
    return ::BeginPaint(g_gameApp.GetMainWindow(), pPaint);
}

// FUNCTION: HYPERX 0x0048f380
HPALETTE GDI::SelectSplashPalette(HDC hdc, HPALETTE hPalette, BOOL bForceBackground)
{
    g_hSplashPalette = hPalette;

    return SelectPalette(hdc, hPalette, bForceBackground);
}

// FUNCTION: HYPERX 0x0048f3a0
UINT GDI::RealizePalette(HDC hdc)
{
    return ::RealizePalette(hdc);
}

// FUNCTION: HYPERX 0x0048f3b0
void GDI::DrawTransparentText(HDC hdc, COLORREF color, int x, int y, const char* pszText, int cchText, int)
{
    SetTextAlign(hdc, 0);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);

    TextOutA(hdc, x, y, pszText, cchText);
}

// FUNCTION: HYPERX 0x0048f3f0
BOOL GDI::EndPaint(const PAINTSTRUCT* pPaint)
{
    return ::EndPaint(g_gameApp.m_hWnd, pPaint);
}

// FUNCTION: HYPERX 0x0048f410
void GDI::SafeDeleteBitmap(HBITMAP hBitmap)
{
    if (hBitmap != 0)
    {
        ::DeleteObject(hBitmap);
    }
}

// FUNCTION: HYPERX 0x0048f420
BOOL GDI::DeleteObject(HGDIOBJ hObject)
{
    return ::DeleteObject(hObject);
}

// FUNCTION: HYPERX 0x0048f430
void GDI::GetBitmapDimensions(HBITMAP hBitmap, BITMAP* pBitmap)
{
    GetObjectA(hBitmap, sizeof(BITMAP), pBitmap);
}

// The raster operation is not read: the blit is always a straight copy.
// FUNCTION: HYPERX 0x0048f4b0
int GDI::BlitBitmapToDC(HDC hdc, int x, int y, int cx, int cy, HBITMAP hBitmap, HPALETTE hPalette, int x1, int y1, DWORD)
{
    const HDC hdcMem = CreateCompatibleDC(hdc);

    HPALETTE hOldPalette = GDI::SelectSplashPalette(hdcMem, hPalette, FALSE);

    GDI::RealizePalette(hdcMem);

    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hBitmap);

    BitBlt(hdc, x, y, cx, cy, hdcMem, x1, y1, SRCCOPY);

    GDI::SelectSplashPalette(hdcMem, hOldPalette, FALSE);

    SelectObject(hdcMem, hOldBitmap);

    DeleteDC(hdcMem);

    return 1;
}

// The JPEG loader returns a device bitmap and no palette.
// FUNCTION: HYPERX 0x0048f540
HBITMAP GDI::LoadBitmapWithPalette(LPCSTR pszResourceName, HPALETTE* phPalette)
{
    int        cColors = 256;
    PathString strPath(MAX_PATH);
    HBITMAP    hbm;
    HPALETTE   hOldPalette;

    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, pszResourceName, "bmp");

    if (_access(strPath.m_str.m_pchData, 0) == -1)
    {
        // STRING: HYPERX 0x004c8960
        strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, pszResourceName, "jpg");

        if (_access(strPath.m_str.m_pchData, 0) == -1)
        {
            return 0;
        }

        return GDI::LoadJPEGImage(strPath.m_str.m_pchData);
    }

    BITMAPINFO* const pDib = (BITMAPINFO*)GDI::ReadBitmapFile(strPath.m_str.m_pchData);

    if (pDib != 0)
    {
        const HDC hdc = GetDC(g_gameApp.m_hWnd);

        HPALETTE hPalette = GDI::CreatePaletteFromDIB(pDib, &cColors);

        if (hPalette != 0)
        {
            hOldPalette = GDI::SelectSplashPalette(hdc, hPalette, 0);
            GDI::RealizePalette(hdc);
        }
        else
        {
            hOldPalette = 0;
        }

        const RGBQUAD* const pBits = GDI::ColorTable(&pDib->bmiHeader) + cColors;

        hbm = CreateDIBitmap(hdc, (BITMAPINFOHEADER*)pDib, CBM_INIT, pBits, pDib, DIB_RGB_COLORS);

        free(pDib);

        GDI::SelectSplashPalette(hdc, hOldPalette, 0);

        if (hPalette != 0)
        {
            if (phPalette != 0)
            {
                *phPalette = hPalette;
            }
            else
            {
                ::DeleteObject(hPalette);
            }
        }

        ReleaseDC(0, hdc);

        if (hbm == 0)
        {
            // STRING: HYPERX 0x004bc42c
            g_gameApp.Error("Problem Creating DIBitmap %s, check palette!", pszResourceName);
        }

        return hbm;
    }

    return 0;
}

// Entries 10 to 245 are marked reserved, leaving the system colours in place.
// FUNCTION: HYPERX 0x0048f720
HPALETTE GDI::CreatePaletteFromDIB(BITMAPINFO* pbmi, int* pcColors)
{
    HPALETTE hPalette = 0;

    if (pbmi->bmiHeader.biBitCount <= 8)
    {
        *pcColors = 1 << pbmi->bmiHeader.biBitCount;
    }
    else
    {
        *pcColors = 0;
    }

    if (*pcColors != 0)
    {
        LOGPALETTE* const pLog = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + *pcColors * sizeof(PALETTEENTRY));

        pLog->palVersion    = 0x300;
        pLog->palNumEntries = (WORD)*pcColors;

        for (int i = 0; i < *pcColors; i++)
        {
            pLog->palPalEntry[i].peRed   = pbmi->bmiColors[i].rgbRed;
            pLog->palPalEntry[i].peGreen = pbmi->bmiColors[i].rgbGreen;
            pLog->palPalEntry[i].peBlue  = pbmi->bmiColors[i].rgbBlue;

            if (i > 9 && i < 246)
            {
                pLog->palPalEntry[i].peFlags = PC_RESERVED;
            }
            else
            {
                pLog->palPalEntry[i].peFlags = 0;
            }
        }

        hPalette = CreatePalette(pLog);

        free(pLog);
    }

    return hPalette;
}

// A 24-bit bottom-up DIB section; stb returns top-down RGB, so rows are flipped and swapped to BGR.
// FUNCTION: HYPERX 0x0048f7d0
HBITMAP GDI::LoadJPEGImage(const char* pszPath)
{
    int nWidth    = 0;
    int nHeight   = 0;
    int nChannels = 0;

    stbi_uc* const pPixels = stbi_load(pszPath, &nWidth, &nHeight, &nChannels, 3);

    if (pPixels == 0)
    {
        return 0;
    }

    BITMAPINFO bmi              = {};
    bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth       = nWidth;
    bmi.bmiHeader.biHeight      = nHeight;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    void*         pBits   = 0;
    const HBITMAP hBitmap = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, &pBits, 0, 0);

    if (hBitmap != 0)
    {
        const auto nStride = (nWidth * 3 + 3) & ~3;

        for (int y = 0; y < nHeight; y++)
        {
            const stbi_uc* pSrc  = pPixels + (size_t)(nHeight - 1 - y) * nWidth * 3;
            unsigned char* pDest = (unsigned char*)pBits + (size_t)y * nStride;

            for (int x = 0; x < nWidth; x++)
            {
                pDest[0] = pSrc[2];
                pDest[1] = pSrc[1];
                pDest[2] = pSrc[0];
                pDest += 3;
                pSrc += 3;
            }
        }
    }

    stbi_image_free(pPixels);

    return hBitmap;
}
