#ifndef HYPERBLADE_STEPPEDSCROLLBITMAP_H
#define HYPERBLADE_STEPPEDSCROLLBITMAP_H

#include "common/compat.h"
#include "graphics/scrollbitmap.h"

// Fills its scroll band in steps that rise across it.
class SteppedScrollBitmap : public ScrollBitmap
{
public:
    int InitAndLoad(const char* pszName, int nCount, int bColourKey, int bSystemMemory);

    SteppedScrollBitmap()
    {
    }

    SteppedScrollBitmap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
    : ScrollBitmap(pDDraw, pPrimary, pBack)
    {
    }

    virtual int DrawScrolled();

public:
    int m_nStepCount;  // 0x10c
    int m_nStepX;      // 0x110
    int m_nStepY;      // 0x114
    int m_nBandHeight; // 0x118

    void SetDirty();
};

#endif
