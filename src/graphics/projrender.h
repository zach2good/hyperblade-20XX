#ifndef HYPERBLADE_PROJRENDER_H
#define HYPERBLADE_PROJRENDER_H

// The view volume a camera projects into: the X and Y maxima are the clip half extents.
struct ClipExtents
{
    float flMinX;
    float flMaxX;
    float flMinY;
    float flMaxY;
    float flMinZ;
    float flMaxZ;
};

#endif
