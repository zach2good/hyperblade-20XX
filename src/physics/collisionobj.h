#ifndef HYPERBLADE_COLLISIONOBJ_H
#define HYPERBLADE_COLLISIONOBJ_H

#include "engine/gameengine.h"

#include "common/compat.h"
#include "collisionplane.h"

// VTABLE: HYPERX 0x004b155c
class CollisionObj : public CollisionPlane
{
public:
    static int GetModelId(int nIndex);

public:
    // 0x00405850 stores this table and jumps to ~CollisionShape, skipping CollisionPlane.
    // FUNCTION: HYPERX 0x00405850
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

// SYNTHETIC: HYPERX 0x004807b0
// CollisionObj::operator=

#endif
