// The collision ellipsoid: three radii, then one over each squared.

#ifndef HYPERBLADE_COLLISIONELLIPSOID_H
#define HYPERBLADE_COLLISIONELLIPSOID_H

#include "engine/gameengine.h"
#include "collisionshape.h"

struct Vec3;

// A sibling of CollisionPlane: this table has thirteen slots to the plane's fourteen.
// VTABLE: HYPERX 0x004b16bc
class CollisionEllipsoid : public CollisionShape
{
public:
    CollisionEllipsoid(float flX, float flY, float flZ, float flFriction, float flElasticity);
    ~CollisionEllipsoid();

    // Call sites on an ellipsoid are qualified; the original calls the arena's slot one directly.
    float GetZ(float flX, float flY) override;

    int IntersectRay(Vec3* pDirection, Vec3* pOrigin, Vec3* pNearOut, Vec3* pFarOut);
    int SegmentIntersection(Vec3* pFrom, Vec3* pTo, Vec3* pOut) override;
    int IntersectSegment(Vec3* pFirst, Vec3* pSecond, Vec3* pNearOut, Vec3* pFarOut);
    // Must override the slot: left pure, the arena ellipsoid takes every terrain ray into R6025.
    int RayIntersection(Vec3* pDirection, Vec3* pOrigin, Vec3* pPointOut) override;

    Vec3* NormalAtPoint(const Vec3* pPoint) override;

    // Only the semi-axes and their reciprocals above are read by any of the four.
    float         RadiusForDirection(const Vec3* pVDir, int nUnused) override;
    void          ComputeRadialBoundaryXY(Vec3* pIn, Vec3* pOut);
    unsigned char PointInside(const Vec3* pPoint) override;
    int           PointNearSurface(const Vec3* pPoint) override;

    float GetRadiusX() const
    {
        return m_flRadiusX;
    }

    float GetRadiusY() const
    {
        return m_flRadiusY;
    }

    float GetRadiusZ() const
    {
        return m_flRadiusZ;
    }

    const float& GetInvXX() const
    {
        return m_flInvXX;
    }

    const float& GetInvYY() const
    {
        return m_flInvYY;
    }

    const float& GetInvZZ() const
    {
        return m_flInvZZ;
    }

    float m_flRadiusX; // 0x38
    float m_flRadiusY; // 0x3c
    float m_flRadiusZ; // 0x40
    float m_flInvXX;   // 0x44
    float m_flInvYY;   // 0x48
    float m_flInvZZ;   // 0x4c
};

// The trackers' own ellipsoid, 2.2 inside the arena's so the camera steers inside the wall.
// GLOBAL: HYPERX 0x004cb3d0
extern CollisionEllipsoid g_camTrackEllipsoid;
// SYNTHETIC: HYPERX 0x00408e70
// $E12

// SYNTHETIC: HYPERX 0x00408e80
// $E11

// GLOBAL: HYPERX 0x004ceff8
extern CollisionEllipsoid g_arenaEllipsoid;
// SYNTHETIC: HYPERX 0x0045c6c0
// $E12

#endif
