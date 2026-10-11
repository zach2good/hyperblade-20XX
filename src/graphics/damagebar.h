#ifndef HYPERBLADE_DAMAGEBAR_H
#define HYPERBLADE_DAMAGEBAR_H

#include "steppedscrollbitmap.h"

#include "common/compat.h"
#include "texture.h"
#include "scrollbitmap.h"

// CDDBitMap's twenty seven slots and seven of its own.

// VTABLE: HYPERX 0x004b2f84
class DamageBar : public SteppedScrollBitmap
{
public:
    virtual ~DamageBar();

    DamageBar()
    {
    }

    DamageBar(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
    : SteppedScrollBitmap(pDDraw, pPrimary, pBack)
    {
    }
};

// SYNTHETIC: HYPERX 0x0043cc50
// DamageBar::`scalar deleting destructor'

#endif
