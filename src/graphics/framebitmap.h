#ifdef HYPERX_V100

// A bitmap that shows one of several frames, loaded from numbered files. Nothing in v1.0.0.0 builds one.
#ifndef HYPERBLADE_FRAMEBITMAP_H
#define HYPERBLADE_FRAMEBITMAP_H

#include "common/compat.h"
#include "graphics/cddbitmap.h"

class FrameBitmap : public CDDBitMap
{
public:
    FrameBitmap();
    FrameBitmap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack);
    virtual ~FrameBitmap();

    virtual int RestoreContents();

    static HPALETTE CreatePaletteFromDIB(BITMAPINFO* pbmi, int* pcColors);

    void          ClearState();
    unsigned char LoadFrames(const char* pszBaseName, int nCount, int bColourKey, int);
    int           BltFrame(RECT rcDest);
    int           SetFrame(unsigned char nFrame, RECT rcDest);

    CDDBitMap**   m_ppFrames;    // 0x0d8
    unsigned char m_nFrameCount; // 0x0dc
    unsigned char m_nFrame;      // 0x0dd
};

#endif

#endif
