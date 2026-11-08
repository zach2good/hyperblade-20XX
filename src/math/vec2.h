#ifndef HYPERBLADE_VEC2_H
#define HYPERBLADE_VEC2_H

#ifdef HYPERX_V100
// TODO: The name and type are guesses; only the two dead normalise helpers use it.
struct Vec2
{
    float x;
    float y;

    // Answers false and zeroes the vector when its length is at most 1e-7.
    bool Normalize();
    bool SetNormalized(float flX, float flY);
};
#endif

#endif
