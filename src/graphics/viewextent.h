#ifndef HYPERBLADE_VIEWEXTENT_H
#define HYPERBLADE_VIEWEXTENT_H

#include "common/compat.h"
#include "math/point.h"

struct ViewExtent : public Point
{
    // Non-POD, so it is returned through a hidden pointer as in the original.
    ViewExtent() = default;

    ViewExtent(const ViewExtent& other)
    {
        Point::operator=(other);
    }

    ViewExtent& operator=(const Point& pt)
    {
        Point::operator=(pt);

        return *this;
    }

    // The second argument is never read.
    void ShrinkClamp(int nAmount, int nUnused);
    void ShrinkClamp(Point amount);
};

#endif
