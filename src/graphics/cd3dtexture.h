#ifndef HYPERBLADE_CD3DTEXTURE_H
#define HYPERBLADE_CD3DTEXTURE_H

#include "common/compat.h"
#include "graphics/texture.h"
#include "globals.h"
#include "common/string.h"
#include "graphics/graphicsglobals.h"
#include <d3d.h>

// VTABLE: HYPERX 0x004b2cc4
class CD3DTexture : public Texture
{
public:
    virtual ~CD3DTexture();
    int                         Restore(const char* pName);
    virtual int                 OnSurfaceLost();
    virtual int                 RestoreContents();
    virtual IDirectDrawSurface* CreateSurface(int nWidth, int nHeight, int bColourKey, int bSystemMemory);

    CD3DTexture(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack);
    int           Create(const char* pszName, int bColourKey, int bSystemMemory);
    unsigned char LoadBitmapFile(const char* pszName);
    int           CreateVidTexture(IDirectDrawSurface* pSource, IDirectDrawSurface** ppSurface, IDirect3DTexture** ppTexture);
    void          ClearRecord();

    unsigned int GetTextureHandle() const
    {
        return m_dwHandle;
    }

    IDirectDrawSurface* GetVideoSurface() const
    {
        return m_pDestSurface;
    }

private:
    String m_strName; // 0x0c0
    // The test of biBitCount against 8 and 24 asks whether the file is paletted or true colour.
    BITMAPINFOHEADER* m_pBitmapRecord; // 0x0c4
    char*             m_pPixels;       // 0x0c8
    HPALETTE          m_hPalette;      // 0x0cc
    int               m_nRowBytes;     // 0x0d0
    int               m_nWidthLog2;    // 0x0d4
    int               m_nRowMask;      // 0x0d8
    int               m_nColumnBytes;  // 0x0dc
    int               m_nHeightLog2;   // 0x0e0
    int               m_nColumnMask;   // 0x0e4
    // A positive biHeight: the bitmap's rows run bottom up.
    char                m_bBottomUp;       // 0x0e8
    int                 m_nUnusedC;        // 0x0ec
    IDirect3DTexture*   m_pTextureSurface; // 0x0f0, the system-memory surface as a texture, handed to Load
    D3DTEXTUREHANDLE    m_dwHandle;        // 0x0f4
    IDirect3DTexture*   m_pDest;           // 0x0f8
    IDirectDrawSurface* m_pDestSurface;    // 0x0fc

    static int __cdecl GetLog2Size(unsigned int nSize);
};

// SYNTHETIC: HYPERX 0x00433970
// CD3DTexture::`scalar deleting destructor'

#endif
