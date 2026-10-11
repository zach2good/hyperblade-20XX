#ifndef HYPERBLADE_CDDDRAWABLE_H
#define HYPERBLADE_CDDDRAWABLE_H
#include "common/compat.h"
#include "graphics/surfacepoint.h"

// The root of the 2D items drawn on screen: text, sprites, surfaces and the info bar slots.
// VTABLE: HYPERX 0x004b32bc
class CDDDrawable
{
public:
    virtual ~CDDDrawable()
    {
    }

    virtual int Draw() = 0;

    // FUNCTION: HYPERX 0x00451b90
    CDDDrawable()
    {
        m_ptDrawOffset.y = 0;
        m_ptDrawOffset.x = 0;
        m_ptPosition.y   = 0;
        m_ptPosition.x   = 0;
        m_ptSize.y       = 0;
        m_ptSize.x       = 0;
    }

    const SurfacePoint& GetSize() const
    {
        return m_ptSize;
    }

    void SetPosition(const Point& pt)
    {
        m_ptPosition = pt;
    }

    SurfacePoint m_ptSize;       // 0x04
    SurfacePoint m_ptPosition;   // 0x0c
    SurfacePoint m_ptDrawOffset; // 0x14
};

// SYNTHETIC: HYPERX 0x0044f1c0
// CDDDrawable::`scalar deleting destructor'

#endif
