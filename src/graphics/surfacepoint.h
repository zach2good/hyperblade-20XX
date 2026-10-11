// A point that starts at zero, as every surface's size, position and inset do, or at a given place.

#ifndef HYPERBLADE_SURFACEPOINT_H
#define HYPERBLADE_SURFACEPOINT_H

#include "math/point.h"

struct SurfacePoint : public Point
{
    SurfacePoint()
    {
        Point::Zero();
    }

    SurfacePoint(int nX, int nY)
    {
        Point::Set(nX, nY);
    }

    explicit SurfacePoint(Point pt)
    {
        Point::operator=(pt);
    }

    SurfacePoint& operator=(const Point& pt)
    {
        Point::operator=(pt);

        return *this;
    }
};

#endif
