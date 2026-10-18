#ifndef HYPERBLADE_INFOBARSLOT_H
#define HYPERBLADE_INFOBARSLOT_H

#include "common/compat.h"
#include "graphics/rlespritesurface.h"

// Its destructor puts the shared surface table back and does nothing else.
// VTABLE: HYPERX 0x004b3350
class InfoBarSlot : public CDDDrawable
{
public:
    // FUNCTION: HYPERX 0x00451be0
    InfoBarSlot()
    : m_nFrameCount(0)
    , m_nFrame(0)
    , m_nMinHeight(0)
    , m_nMaxHeight(0)
    , m_nUnitWidth(0)
    , m_nColour(0)
    , m_flScalar(0)
    {
    }

    // Inline because the original inlines it into CDDSurfaceOwner's and InfoBar's destructors.
    // FUNCTION: HYPERX 0x0044f1b0
    ~InfoBarSlot() noexcept override = default;

    int Draw() override;

    void SetStrip(int nFrameCount, int nUnitWidth, int nMinHeight, int nMaxHeight)
    {
        m_flScalar    = 0;
        m_nFrameCount = nFrameCount;
        m_nUnitWidth  = nUnitWidth;
        m_nMinHeight  = nMinHeight;
        m_nMaxHeight  = nMaxHeight;
    }

    // infobar.cpp seeds m_nMinHeight, m_nMaxHeight and m_nUnitWidth with two, eight and nine for each damage bar.
    int   m_nFrameCount; // 0x1c
    int   m_nFrame;      // 0x20
    int   m_nMinHeight;  // 0x24
    int   m_nMaxHeight;  // 0x28
    int   m_nUnitWidth;  // 0x2c
    int   m_nColour;     // 0x30
    float m_flScalar;    // 0x34
};

// SYNTHETIC: HYPERX 0x00451c40
// InfoBarSlot::`scalar deleting destructor'

#endif
