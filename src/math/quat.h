#ifndef HYPERBLADE_QUAT_H
#define HYPERBLADE_QUAT_H

class TransformObject;

class Quat
{
public:
    static bool Normalize(float* pW, float* pX, float* pY, float* pZ);

public:
    void FromMatrix(TransformObject* pSource);
    void ToMatrix(TransformObject* pTarget);

    float w;
    float x;
    float y;
    float z;
};

#endif
