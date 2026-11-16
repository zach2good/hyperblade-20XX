#ifndef HYPERBLADE_COLLISIONOBJ_H
#define HYPERBLADE_COLLISIONOBJ_H

#include "engine/gameengine.h"

#include "common/compat.h"
#include "physics/collisionshape.h"
#include "globals.h"

// VTABLE: HYPERX 0x004b155c
class CollisionObj : public CollisionShape
{
public:
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

    static int __cdecl GetModelId(int nIndex);

public:
    // 0x00405850 stores this table and jumps to ~CollisionShape.
    // FUNCTION: HYPERX100 0x00412270
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

    // RampObject's first three parts inline this and the copy below; its fourth calls the
    // out-of-line forms.
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
