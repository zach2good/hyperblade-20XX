#ifndef HYPERBLADE_SOFTTEX_H
#define HYPERBLADE_SOFTTEX_H

#include "common/compat.h"
#include <stddef.h>

struct RLESprite;

// What the texture library's span routine reads at the start of a SoftTex.
struct SoftTexImage
{
    int   nWidth;  // 0x00
    int   nHeight; // 0x04
    int   nStride; // 0x08
    int   nBpp;    // 0x0c
    char* pPixels; // 0x10, past the DIB's colour table
};

class SoftTex
{
public:
    // Handed a loaded DIB, not a SoftTex.
    static RLESprite* CompressToRLE(const BITMAPINFOHEADER* pDib);

public:
    SoftTex();

    ~SoftTex()
    {
        RemoveFromList();
    }

    void RemoveFromList();

    // The second argument is never read.
    char Load(const char* pszName, HPALETTE hUnused);

    // Hands the polygon to the library's textured span routine, the texture in ECX.
    void DrawTexPoly(class TransformObject* pCamera, struct FacePolyBuffer* pPoly, float flLight);

    int GetWidth() const;
    int GetHeight() const;

private:
    SoftTexImage      m_image; // 0x00
    BITMAPINFOHEADER* m_pDib;  // 0x14
    class SoftTex*    m_pNext; // 0x18

    friend struct SoftTexLayout;
};

struct SoftTexLayout
{
};

#endif
