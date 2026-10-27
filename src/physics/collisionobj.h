#ifndef HYPERBLADE_COLLISIONOBJ_H
#define HYPERBLADE_COLLISIONOBJ_H

#include "engine/gameengine.h"

#include "common/compat.h"
#include "collisionplane.h"

// VTABLE: HYPERX 0x004b155c
class CollisionObj : public CollisionPlane
{
public:
    static int __cdecl GetModelId(int nIndex);

public:
    // 0x00405850 stores this table and jumps to ~CollisionShape, skipping CollisionPlane.
    // FUNCTION: HYPERX 0x00405850
    ~CollisionObj() throw()
    {
    }

    // What RampObject's four parts are built with.
    CollisionObj()
    {
        m_vPoint.z     = g_flCollisionPartDefault;
        m_vPoint.y     = g_flCollisionPartDefault;
        m_vPoint.x     = g_flCollisionPartDefault;
        m_nShapeKind   = SHAPE_KIND_PLANE;
        m_flFriction   = g_flCollisionPartDefault;
        m_flElasticity = g_flCollisionPartDefault;
    }

    CollisionObj(const Vec3* pNormal, const Vec3* pOrigin, float flFriction, float flElasticity);

    CollisionObj(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity);

    // RampObject's first three parts inline this and the copy below; its fourth calls the out-of-line forms.
    CollisionObj(const Vec3& vOrigin, const Vec3& vB, const Vec3& vC, float flFriction, float flElasticity)
    {
        InitFrom3Points(&vOrigin, &vB, &vC, flFriction, flElasticity);
    }

    CollisionObj& operator=(const CollisionObj& other);

    // The base's copy, then this class's own fields.
    void CopyInline(const CollisionObj& other)
    {
        CollisionShape::operator=(other);

        m_vPoint   = other.m_vPoint;
        m_flSlopeX = other.m_flSlopeX;
        m_flSlopeY = other.m_flSlopeY;
        m_flOffset = other.m_flOffset;
        m_bCached  = other.m_bCached;
    }

    void InitFrom3Points(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity);
};

#endif
