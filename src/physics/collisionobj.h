#ifndef HYPERBLADE_COLLISIONOBJ_H
#define HYPERBLADE_COLLISIONOBJ_H

#include "engine/gameengine.h"

#include "common/compat.h"
#include "physics/collisionshape.h"
#include "globals.h"

// VTABLE: HYPERX1001 0x004b155c
class CollisionObj : public CollisionShape
{
public:
    // The seven slots the base leaves pure, plus a fourteenth of this class's own.
    float GetZ(float x, float y) override; // 0x0042e930
    // The two plan collision responses read this slot as a constrain; the reading is unsettled.
    int           RayIntersection(Vec3* pDir, Vec3* pOrigin, Vec3* pHitOut) override;                  // 0x0042e810
    int           SegmentIntersection(Vec3* pFrom, Vec3* pTo, Vec3* pOut) override;                    // 0x0042e7d0
    unsigned char PointInside(const Vec3* pPoint) override;                                            // 0x004058c0
    int           PointNearSurface(const Vec3* pPoint) override;                                       // 0x004058d0
    Vec3*         NormalAtPoint(const Vec3* pPoint) override;                                          // 0x00405860
    float         RadiusForDirection(const Vec3* pVDir, int nUnused) override;                         // 0x00405910
    virtual void  ForwardTransformRequest(int nA, int nB, float flC, float flD, float flE, float flF); // 0x00405880

    // The point on the plane; the normal is the base's m_vNormal.
    Vec3 m_vPoint; // 0x038

    // The plane equation, cached: GetZ fills it once and raises the byte.
    // Deviation: the three-point constructor left the byte to whatever the stack held.
    float         m_flSlopeX = 0.0f; // 0x044
    float         m_flSlopeY = 0.0f; // 0x048
    float         m_flOffset = 0.0f; // 0x04c
    unsigned char m_bCached  = 0;    // 0x050

    static int GetModelId(int nIndex);

public:
    // 0x00405850 stores this table and jumps to ~CollisionShape.
    // FUNCTION: HYPERX1001 0x00405850
    ~CollisionObj() noexcept = default;

    CollisionObj(const CollisionObj&)                = delete;
    CollisionObj& operator=(const CollisionObj&)     = delete;
    CollisionObj(CollisionObj&&) noexcept            = default;
    CollisionObj& operator=(CollisionObj&&) noexcept = default;

    // What RampObject's four parts are built with.
    CollisionObj()
    {
        m_vPoint.z     = g_flCollisionPartDefault;
        m_vPoint.y     = g_flCollisionPartDefault;
        m_vPoint.x     = g_flCollisionPartDefault;
        m_nShapeKind   = ShapeKind::kPlane;
        m_flFriction   = g_flCollisionPartDefault;
        m_flElasticity = g_flCollisionPartDefault;
    }

    CollisionObj(const Vec3* pNormal, const Vec3* pOrigin, float flFriction, float flElasticity);

    CollisionObj(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity);
    void InitFrom3Points(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity);
};

// SYNTHETIC: HYPERX1001 0x004807b0
// CollisionObj::operator=

#endif
