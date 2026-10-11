// Shrinks a size in both directions, clamped at zero; the second argument is unread.

#include "common/compat.h"
#include "graphics/renderer.h"
#include "graphics/viewextent.h"

// FUNCTION: HYPERX 0x0042f9a0
void ViewExtent::ShrinkClamp(int nAmount, int)
{
    x -= nAmount;

    if (x < 0)
    {
        x = 0;
    }

    y -= nAmount;

    if (y < 0)
    {
        y = 0;
    }
}

// The original's link folds this onto the body above.
void ViewExtent::ShrinkClamp(Point amount)
{
    x -= amount.x;

    if (x < 0)
    {
        x = 0;
    }

    y -= amount.x;

    if (y < 0)
    {
        y = 0;
    }
}
