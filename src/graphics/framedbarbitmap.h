#ifndef HYPERBLADE_FRAMEDBARBITMAP_H
#define HYPERBLADE_FRAMEDBARBITMAP_H

#include "common/compat.h"
#include "graphics/cddbitmap.h"
#include "common/commonglobals.h"
#include "graphics/rlespritesurface.h"
#include "globals.h"
#include "math/point.h"

// A frame bitmap over a bar bitmap drawn clipped to a width, the speed meter.
// Derives from CDDBitMap and repeats ScrollBitmap's run from 0xd8 to 0x108; shared bodies folded.
// SLOT TABLE 0x004b2f08
// VTABLE: HYPERX 0x004b2f08
class FramedBarBitmap : public CDDBitMap
{
public:
    virtual ~FramedBarBitmap();

    // ScrollBitmap carries the markers for slots four and twenty: the original's linker folded each pair.
    virtual int BltFillColour(unsigned int dwFillColor)
    {
        RECT rc;

        if (m_bHorizontal != 0)
        {
            m_rcScroll.right = m_ptSize.x - m_ptInset.x;
        }
        else
        {
            m_rcScroll.top = m_ptInset.y;
        }

        m_bltfx.dwFillColor = dwFillColor;

        if (m_pSurface == 0)
        {
            return 0;
        }

        if ((m_dwSurfaceCaps & DDSCAPS_OFFSCREENPLAIN) != 0)
        {
            m_hr = m_pSurface->Blt(&m_rcScroll, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
        }
        else
        {
            SetRect(&rc, m_ptPosition.x + m_rcScroll.left, m_ptPosition.y + m_rcScroll.top, m_ptPosition.x + m_rcScroll.right, m_ptPosition.y + m_rcScroll.bottom);

            m_hr = g_pSurfaceBack->Blt(&rc, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_bltfx);
        }

        if (m_hr == DDERR_SURFACELOST)
        {
            RestoreContents();
        }

        return m_hr == 0;
    }

    virtual void SetPosition(Point pt);
    virtual void SetScrollColorMode(int nScrollColor);

    virtual void SetClipRect(int nClipped, Point pt)
    {
        CDDBitMap::SetClipRect(nClipped, pt);

        const SurfacePoint* const pSteps = &m_ptSteps;
        const int                 nStepY = pSteps->y;

        m_ptSteps.x = pSteps->x;
        m_ptSteps.y = nStepY;

        if (m_bHorizontal != 0)
        {
            m_flStep = (float)(m_ptSize.x - m_ptInset.x * 2) / (float)(unsigned int)m_ptSteps.y;
        }
        else
        {
            m_flStep = (float)(m_ptSize.y - m_ptInset.y * 2) / (float)(unsigned int)m_ptSteps.y;
        }

        if (m_bClipped != 0)
        {
            m_nScrollEdge = m_ptInset.x;

            m_rcScroll = *m_pDestRect;
        }
    }

    virtual int RestoreContents();

    FramedBarBitmap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack);

    // The second argument is stored and never read; the fourth is never read.
    int LoadFromFile(const char* pszName, int nUnused, int bColourKey, int);

    void ClearState();
    int  DrawWithWidth(int nWidth);

public:
    SurfacePoint m_ptSteps;    // 0x0d8
    float        m_flStep;     // 0x0e0
    int          m_nScrollPos; // 0x0e4
    int          m_nUnusedA;   // 0x0e8
    // ClearState sets it to one, and nothing reads it.
    int  m_nUnusedB;    // 0x0ec
    int  m_nScrollEdge; // 0x0f0
    int  m_bHorizontal; // 0x0f4
    RECT m_rcScroll;    // 0x0f8
    // LoadFromFile stores its second argument here, and nothing reads it.
    int              m_nUnusedC; // 0x108
    class CDDBitMap* m_pBar;     // 0x10c
};

// SYNTHETIC: HYPERX 0x00439160
// FramedBarBitmap::`scalar deleting destructor'

#endif
