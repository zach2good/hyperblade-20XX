#ifndef HYPERBLADE_CDDBITMAP_H
#define HYPERBLADE_CDDBITMAP_H

#include "texture.h"
#include "math/point.h"

class String;

// VTABLE: HYPERX 0x004b2e2c
class CDDBitMap : public Texture
{
public:
    virtual ~CDDBitMap();
    virtual int  OnSurfaceLost();
    virtual int  Draw();
    virtual void SetPosition(Point pt);
    virtual void SetClipRect(int nClipped, Point pt);
    virtual int  RestoreContents();
    virtual int  CopyBitmap(HBITMAP hBitmap);

    CDDBitMap();
    explicit CDDBitMap(Texture* pSource);
    CDDBitMap(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack);

    int  CreateFromHBitmap(HBITMAP hBitmap, int bColourKey, int bSystemMemory);
    int  LoadFromFile(const char* pszName, int bColourKey, int bSystemMemory);
    void SetFile(const char* pszName, int bColourKey);

    int SetTextOverlay(const char* pszName, int nSize, const char* pszText, int bColourKey);

public:
    RECT                  m_rcBounds; // 0x0c0
    class CDDTextSurface* m_pOverlay; // 0x0d0
    String*               m_pName;    // 0x0d4

    void ClearBounds();
};

// SYNTHETIC: HYPERX 0x00437f80
// CDDBitMap::`scalar deleting destructor'

#endif
