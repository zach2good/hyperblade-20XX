#ifndef HYPERBLADE_VEC3_H
#define HYPERBLADE_VEC3_H

#include <math.h>

struct Vec3
{
    static void  ScaleTo(const Vec3* pIn, float flScale, Vec3* pOut);
    static Vec3* FindClosest(Vec3* pFrom, Vec3* pFirst, Vec3* pSecond);
    static void  CrossProduct(const Vec3* pA, const Vec3* pB, Vec3* pOut);
    static float Dot(const Vec3* pA, const Vec3* pB);
    static bool  NormalizeScattered(float* pX, float* pY, float* pZ);
    static float Length(const Vec3* pVec);

    float x;
    float y;
    float z;

    Vec3* ScaleInPlace(float flScale);
    Vec3* Subtract(Vec3* pOut, const Vec3* pB) const;

    bool NormalizeTo(Vec3* pOut) const;

    // Inline in the original; Dot is its out-of-line copy.
    static float DotInline(const Vec3* pA, const Vec3* pB)
    {
        return pA->x * pB->x + pA->y * pB->y + pA->z * pB->z;
    }

    float Magnitude() const
    {
        return static_cast<float>(sqrt(static_cast<double>(DotInline(this, this))));
    }

    void Set(float flX, float flY, float flZ)
    {
        x = flX;
        y = flY;
        z = flZ;
    }

    void Add(const Vec3& vDelta)
    {
        x = vDelta.x + x;
        y = vDelta.y + y;
        z = vDelta.z + z;
    }

    float LengthSquared() const
    {
        return z * z + x * x + y * y;
    }

    float GetX() const
    {
        return x;
    }

    float GetY() const
    {
        return y;
    }

    float GetZ() const
    {
        return z;
    }

    // The /Od swept tests build each of these as a temporary.
    Vec3 Scaled(float flScale) const;
    Vec3 Minus(const Vec3* pFrom) const;
    Vec3 Copy() const;
};

inline Vec3 Vec3::Scaled(float flScale) const
{
    Vec3 vOut;

    vOut.x = flScale * x;
    vOut.y = flScale * y;
    vOut.z = flScale * z;

    return vOut;
}

inline Vec3 Vec3::Minus(const Vec3* pFrom) const
{
    Vec3 vOut;

    vOut.x = x - pFrom->x;
    vOut.y = y - pFrom->y;
    vOut.z = z - pFrom->z;

    return vOut;
}

inline Vec3 Vec3::Copy() const
{
    Vec3 vOut;

    vOut = *this;

    return vOut;
}

#endif
