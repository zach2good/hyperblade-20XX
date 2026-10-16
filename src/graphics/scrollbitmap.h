// The scrolling bitmap behind the shot power meter. Every writer of the vtable at 0x004b2e98 reaches it here.

#ifndef HYPERBLADE_SCROLLBITMAP_H
#define HYPERBLADE_SCROLLBITMAP_H

#include "common/compat.h"
#include "graphics/cddbitmap.h"
#include "graphics/rlespritesurface.h"
#include "graphics/surfacepoint.h"

// VTABLE: HYPERX 0x004b2e98
class ScrollBitmap : public CDDBitMap
{
public:
    virtual ~ScrollBitmap();

    // FUNCTION: HYPERX 0x00438b60
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

    virtual void SetScrollColorMode(int nScrollColor);

    // FUNCTION: HYPERX 0x00438940
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
    virtual int DrawScrolled();

    void ClearState();

    // The third argument is never read.
    int LoadFromFile(const char* pszName, int bColourKey, int nUnused);
    int ScrollTo(int nPosition);

    ScrollBitmap();
    ScrollBitmap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack);

    // The lowest position and the number of steps.
    SurfacePoint m_ptSteps;    // 0x0d8
    float        m_flStep;     // 0x0e0
    int          m_nScrollPos; // 0x0e4
    int          m_nUnusedA;   // 0x0e8
    // ClearState sets it to one, and nothing reads it.
    int  m_nUnusedB;    // 0x0ec
    int  m_nScrollEdge; // 0x0f0
    int  m_bHorizontal; // 0x0f4
    RECT m_rcScroll;    // 0x0f8
    int  m_bDirty;      // 0x108
};

// SYNTHETIC: HYPERX 0x00438740
// ScrollBitmap::`scalar deleting destructor'

static_assert(sizeof(ScrollBitmap) == 0x10c);
#endif
