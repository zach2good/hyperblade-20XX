// The base ends at 0xc0, where each derived class puts its own name string.

#ifndef HYPERBLADE_TEXTURE_H
#define HYPERBLADE_TEXTURE_H

#include "common/compat.h"
#include <ddraw.h>
#include "graphics/surfacepoint.h"

struct IDirectDrawSurface;

// GLOBAL: HYPERX 0x004cd4c8
extern LPDIRECTDRAWSURFACE g_pSurfaceBack;

struct TextureRect
{
    int nLeft;
    int nTop;
    int nRight;
    int nBottom;
};

// VTABLE: HYPERX 0x004b2d5c
class Texture
{
public:
    virtual ~Texture();
    virtual int OnSurfaceLost();
    virtual int AllocSurface(SurfacePoint size, int bColourKey, int bSystemMemory);
    virtual int BltFill();
    virtual int BltFillColour(unsigned int nColour);
    virtual int Draw();
    // Empty, never overridden, never called.
    virtual void                Unused(int, int, int);
    virtual void                SetVisible(int nVisible);
    virtual void                SetPosition(Point pt);
    virtual Point               GetPosition();
    virtual void                SetOffset(TextureRect rect);
    virtual void                DispatchRect(int nLeft, int nTop, int nRight, int nBottom);
    virtual IDirectDrawSurface* GetSurface();
    // Nothing calls it.
    virtual Texture* GetSourceSurface();
    virtual Point    GetSize();
    virtual int      GetForeColor();
    virtual int      GetBgColor();
    virtual void     SetColorMode(int nScrollColor);

    // Slot 18 holds slot 17's address in five of the six tables; only ScrollBitmap overrides it.
    virtual void SetScrollColorMode(int nScrollColor);

    virtual void           SetFillColor(int nFillColor);
    virtual void           SetClipRect(int nClipped, Point pt);
    virtual Point          GetInset();
    virtual unsigned char* LockAndGetPitch(int* pnPitch);
    // Slot 23, which forwards to the surface's thirty-third entry, Unlock.
    virtual void Unlock();

    // Slot 24 is KeyConfigDialog::SaveKeysAs in the base, shared with unrelated tables.
    virtual int RestoreContents()
    {
        return 0;
    }

    virtual IDirectDrawSurface* CreateSurface(int nWidth, int nHeight, int bColourKey, int bSystemMemory);

    Texture();
    explicit Texture(Texture* pSource);
    Texture(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack);

    void Init(Texture* pSource);

public:
    int                 m_nUnusedA;       // 0x004
    Texture*            m_pSourceSurface; // 0x008, the surface Init is handed, kept and tested
    IDirectDrawSurface* m_pSurface;       // 0x00c
    // The fill colour lands in the block's own dwFillColor.
    DDBLTFX m_bltfx; // 0x010
    // Bit 6 is DDSCAPS_OFFSCREENPLAIN; clear, the fill goes to the screen surface.
    unsigned int m_dwSurfaceCaps; // 0x074
    SurfacePoint m_ptSize;        // 0x078
    SurfacePoint m_ptPosition;    // 0x080
    LPRECT       m_pDestRect;     // 0x088
    RECT*        m_pClipRect;     // 0x08c
    int          m_bColourKey;    // 0x090
    int          m_nFlags;        // 0x094
    int          m_bClipped;      // 0x098
    SurfacePoint m_ptInset;       // 0x09c
    int          m_bSystemMemory; // 0x0a4
    int          m_bVisible;      // 0x0a8
    DWORD        m_dwScrollColor; // 0x0ac
    DWORD        m_dwFillColor;   // 0x0b0
    DDCOLORKEY   m_colorKey;      // 0x0b4
    HRESULT      m_hr;            // 0x0bc
};

// SYNTHETIC: HYPERX 0x00436df0
// Texture::`scalar deleting destructor'

#endif
