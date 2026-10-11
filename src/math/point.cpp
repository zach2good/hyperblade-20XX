// The point's setters and offsets.

#include "common/compat.h"
#include "math/point.h"

// FUNCTION: HYPERX 0x0042f950
Point* Point::Zero()
{
    x = 0;
    y = 0;

    return this;
}

// FUNCTION: HYPERX 0x0042f960
Point* Point::Set(int nX, int nY)
{
    x = nX;
    y = nY;

    return this;
}

// FUNCTION: HYPERX 0x0042f980
void Point::Offset(int nDx, int nDy)
{
    x += nDx;
    y += nDy;
}

// The original's link folds this onto the body above, so it carries no marker.
void Point::Offset(Point pt)
{
    x += pt.x;
    y += pt.y;
}
