#ifndef HYPERBLADE_CDDTEXTSURFACE_H
#define HYPERBLADE_CDDTEXTSURFACE_H

#include "common/compat.h"
#include "graphics/texture.h"
#include "common/pathstring.h"
#include "common/staticassert.h"

// VTABLE: HYPERX 0x004b2dc4
class CDDTextSurface : public Texture
{
public:
    virtual ~CDDTextSurface();
    virtual int OnSurfaceLost();
    virtual int Draw();
    virtual int RestoreContents();

    int DrawText(const char* pszText);

    void InitFields();

    CDDTextSurface();
    explicit CDDTextSurface(Texture* pSource);

    int SetFont(const char* pszName, int nSize, const char* pszText, int bColourKey, int bSystemMemory);
    int SetFontScaled(const char* pszName, const char* pszText, int bColourKey, int bSystemMemory);

    SurfacePoint RenderText(int bColourKey);
    CDDTextSurface(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack);

public:
    String m_strText; // 0x0c0
    HFONT  m_hFont;   // 0x0c4

    // OPAQUE and ETO_OPAQUE are both 2, so either API could be the first one's intent.
    UINT m_nTextMode;  // 0x0c8
    UINT m_nTextAlign; // 0x0cc
};

// SYNTHETIC: HYPERX 0x00437860
// CDDTextSurface::`scalar deleting destructor'

STATIC_ASSERT(sizeof(CDDTextSurface) == 0xd0);
#endif
