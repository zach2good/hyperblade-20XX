#ifndef HYPERBLADE_POINT_H
#define HYPERBLADE_POINT_H

struct Point
{
    int x;
    int y;

    Point* Set(int nX, int nY);
    void   Offset(int nDx, int nDy);
    void   Offset(Point pt);

    Point* Zero();
};

#endif
