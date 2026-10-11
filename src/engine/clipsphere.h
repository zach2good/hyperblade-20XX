#ifndef HYPERBLADE_CLIPSPHERE_H
#define HYPERBLADE_CLIPSPHERE_H

// The bounding sphere a cull tests; src/graphics/clipsphere.cpp holds the test.
class ClipSphere
{
public:
    char FrustumCull(float flRadius);

    float x;
    float y;
    float z;
    float w;
};

#endif
