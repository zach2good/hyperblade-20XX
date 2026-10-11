#ifndef HYPERBLADE_COLLISIONPLANE_H
#define HYPERBLADE_COLLISIONPLANE_H

#include "common/compat.h"
#include "physics/collisionshape.h"
#include "globals.h"

struct Vec3;

// The table is 0x004b155c; its marker sits on CollisionObj, whose constructors store it.
class CollisionPlane : public CollisionShape
{
public:
    // No destructor: 0x00405850 stores CollisionObj's table and jumps to ~CollisionShape.

    // The seven slots the base leaves pure, plus a fourteenth of this class's own.
    virtual float GetZ(float x, float y); // 0x0042e930
    // The two plan collision responses read this slot as a constrain; the reading is unsettled.
    virtual int           RayIntersection(Vec3* pDir, Vec3* pOrigin, Vec3* pHitOut);                           // 0x0042e810
    virtual int           SegmentIntersection(Vec3* pFrom, Vec3* pTo, Vec3* pOut);                             // 0x0042e7d0
    virtual unsigned char PointInside(const Vec3* pPoint);                                                     // 0x004058c0
    virtual int           PointNearSurface(const Vec3* pPoint);                                                // 0x004058d0
    virtual Vec3*         NormalAtPoint(const Vec3* pPoint);                                                   // 0x00405860
    virtual float         RadiusForDirection(const Vec3* pVDir, int nUnused);                                  // 0x00405910
    virtual void          ForwardTransformRequest(int nA, int nB, float flC, float flD, float flE, float flF); // 0x00405880

    // The point on the plane; the normal is the base's m_vNormal.
    Vec3 m_vPoint; // 0x038

    // The plane equation, cached: GetZ fills it once and raises the byte.
    float         m_flSlopeX; // 0x044
    float         m_flSlopeY; // 0x048
    float         m_flOffset; // 0x04c
    unsigned char m_bCached;  // 0x050
};

#endif
