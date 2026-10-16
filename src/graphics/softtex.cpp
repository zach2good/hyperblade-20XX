#include "common/compat.h"
#include "rlesprite.h"
#include "softtex.h"
#include "graphicsglobals.h"
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "globals.h"
#include "texcacheentry.h"
#include "app/gameapp.h"
#include "graphics/gdi.h"
#include "ui/hudglobals.h"

#define RLE_MAX_RUN 0x7f
#define RLE_LITERAL 0x80

// Top bit set: literal pixels follow; clear: transparent skip. 255 is transparent. The block leaks.
// FUNCTION: HYPERX 0x0044f550
RLESprite* SoftTex::CompressToRLE(const BITMAPINFOHEADER* pDib)
{
    char aRun[128];
    char cPixel  = 0;
    char bOpaque = 0;

    const unsigned int cx = pDib->biWidth;
    const unsigned int cy = pDib->biHeight;

    const char* pPixels = GDI::PixelBits(pDib);

    const unsigned int nStride = (cx + 3) & 0xfffffffc;

    // Room for every row pointer, then each row's worst case of a count byte per pixel and a literal.
    RLESprite* pSprite = (RLESprite*)malloc(sizeof(RLESprite) + cy * sizeof(char*) + (cx + 2) * cy * 2);

    if (pSprite == 0)
    {
        return 0;
    }

    pSprite->cx = cx;
    pSprite->cy = cy;

    char* pOut = (char*)&pSprite->apRows[cy];

    for (unsigned int nRow = 0; nRow < cy; nRow++)
    {
        const char*  pSrc    = pPixels + (cy - nRow - 1) * nStride;
        char         bPrev   = 0;
        unsigned int nColumn = 0;

        pSprite->apRows[nRow] = pOut;

        unsigned int nCount = 0;

        do
        {
            if (nColumn < cx)
            {
                cPixel = *pSrc;
                pSrc++;
                bOpaque = cPixel != -1;
            }

            if (bOpaque != bPrev || nCount == RLE_MAX_RUN || nColumn >= cx)
            {
                if (nCount != 0)
                {
                    if (bPrev == 0)
                    {
                        *pOut = (char)nCount;
                        pOut++;
                    }
                    else
                    {
                        *pOut = (char)(nCount | RLE_LITERAL);
                        pOut++;

                        memcpy(pOut, aRun, nCount);

                        pOut += nCount;
                    }

                    nCount = 0;
                }

                if (nColumn >= cx)
                {
                    break;
                }
            }

            bPrev = bOpaque;

            if (bOpaque == 1)
            {
                aRun[nCount] = cPixel;
            }

            nCount++;
            nColumn++;
        } while (nColumn <= cx);
    }

    // The shrink can move the block, and the row pointers point into it.
    const auto       nOld    = (UINT_PTR)pSprite;
    RLESprite* const pShrunk = (RLESprite*)realloc(pSprite, pOut - (char*)pSprite);

    if (pShrunk == 0)
    {
        return pSprite;
    }

    if ((UINT_PTR)pShrunk != nOld)
    {
        for (unsigned int nRow = 0; nRow < cy; nRow++)
        {
            pShrunk->apRows[nRow] = (char*)pShrunk + ((UINT_PTR)pShrunk->apRows[nRow] - nOld);
        }
    }

    return pShrunk;
}

// The newest texture is unlinked first.
// FUNCTION: HYPERX 0x004928a0
SoftTex::SoftTex()
{
    memset(&m_image, 0, sizeof(m_image));

    m_pDib = 0;

    m_pNext = g_pSoftTexHead;

    g_pSoftTexHead = this;
}

// A texture not in the list sets the head to its own next pointer.
// FUNCTION: HYPERX 0x004928d0
void SoftTex::RemoveFromList()
{
    if (m_pDib != 0)
    {
        free(m_pDib);

        m_pDib = 0;
    }

    SoftTex* pNode = g_pSoftTexHead;
    SoftTex* pPrev = 0;

    while (pNode != 0)
    {
        if (pNode == this)
        {
            break;
        }

        pPrev = pNode;

        pNode = pNode->m_pNext;
    }

    if (pPrev != 0)
    {
        pPrev->m_pNext = m_pNext;

        return;
    }

    g_pSoftTexHead = m_pNext;
}

// The palette is built from the first bitmap loaded, so later textures show through its colours.

// FUNCTION: HYPERX 0x00492920
char SoftTex::Load(const char* pszName, HPALETTE)
{
    char szPath[MAX_PATH];

    if (g_hMdmTexDll != 0)
    {
        strcpy(szPath, g_gameApp.m_strTexturesPath.m_str.m_pchData);
        szPath[strlen(szPath) - 1] = '8';

        // STRING: HYPERX 0x004c8c7c
        strcat(szPath, "\\");
        strcat(szPath, pszName);
        // STRING: HYPERX 0x004bdf7c
        strcat(szPath, ".");
        strcat(szPath, g_pszBitmapExtension);

        m_pDib = GDI::ReadBitmapFile(szPath);

        memset(&m_image, 0, sizeof(m_image));

        if (m_pDib != 0)
        {
            const BITMAPINFOHEADER* pHeader = m_pDib;

            if (pHeader->biBitCount == 8)
            {
                m_image.nWidth  = pHeader->biWidth;
                m_image.nHeight = pHeader->biHeight;
                m_image.nStride = (m_image.nWidth + 3) & ~3;
                m_image.nBpp    = 8;
                m_image.pPixels = GDI::PixelBits(m_pDib);

                if (g_bSoftTexPaletteBuilt == 0)
                {
                    RGBQUAD* pColours;
                    int      i;

                    g_bSoftTexPaletteBuilt = 1;

                    pColours = GDI::ColorTable(m_pDib);

                    // A short table leaves the entries past it black.
                    const auto nEntries = (std::min)((int)((RGBQUAD*)GDI::PixelBits(m_pDib) - pColours), 256);

                    for (i = 0; i < nEntries; i++)
                    {
                        g_aSoftTexPalette[i].peRed   = pColours[i].rgbRed;
                        g_aSoftTexPalette[i].peGreen = pColours[i].rgbGreen;
                        g_aSoftTexPalette[i].peBlue  = pColours[i].rgbBlue;
                        g_aSoftTexPalette[i].peFlags = PC_NOCOLLAPSE;
                    }
                }

                return 1;
            }

            free(m_pDib);
            m_pDib = 0;

            return 0;
        }

        // STRING: HYPERX 0x004c8c58
        g_gameApp.Error("Texture Map cannot be opened!\n%s\n", pszName);
    }

    return 0;
}

#define TEXPOLY_EDGE_COUNT 0x1f

// The entry point is loaded at run time; a null one drops the polygon.
// FUNCTION: HYPERX 0x00492b60
void SoftTex::DrawTexPoly(TransformObject*, FacePolyBuffer* pPoly, float)
{
    if (g_pfnMdmDrawTexPoly != 0)
    {
        g_pfnMdmDrawTexPoly(&m_image, pPoly->aVertices, pPoly->nCount, TEXPOLY_EDGE_COUNT);
    }
}

// FUNCTION: HYPERX 0x00492b40
int SoftTex::GetWidth() const
{
    return m_image.nWidth;
}

// The original's linker folded this onto Widget::GetTypeID, so it has no address of its own.
int SoftTex::GetHeight() const
{
    return m_image.nHeight;
}
