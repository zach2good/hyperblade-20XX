// The D3D HUD's pass bar: a marker texture slid along the bar. Nothing in the image builds one.

#ifndef HYPERBLADE_PASSBAR_H
#define HYPERBLADE_PASSBAR_H

#include "common/compat.h"
#include "graphics/cddbitmap.h"
#include "math/point.h"

class PassBar : public CDDBitMap
{
public:
    int DrawAtPos(int nPosition);
    int AutoScroll();
    int UpdateScrollPos(int nPosition);

    Texture* m_pMarkerTexture; // 0x0d8
    // The lowest and highest position, which GetPassBarSize answers as the bar's size.
    Point m_ptRange;        // 0x0dc
    Point m_ptMarkerOffset; // 0x0e4
    float m_flScale;        // 0x0ec
    int   m_nPosition;      // 0x0f0
    int   m_nAutoPosition;  // 0x0f4
    // Nothing reads or writes it.
    int   m_nUnusedB;    // 0x0f8
    Point m_ptMarker;    // 0x0fc
    int   m_bHorizontal; // 0x104
};

#endif
