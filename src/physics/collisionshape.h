#ifndef HYPERBLADE_COLLISIONSHAPE_H
#define HYPERBLADE_COLLISIONSHAPE_H

#include "common/compat.h"
#include "globals.h"
#include "math/transformobject.h"
#include <memory>

struct Vec3;

// The kind a shape stores; only src/planvm/cprojectandbounce.cpp reads it.
enum class ShapeKind
{
    kPlane     = 0,
    kEllipsoid = 2
};

// VTABLE: HYPERX 0x004b2be0
class CollisionShape
{
public:
    CollisionShape();

    virtual float GetZ(float x, float y)                                    = 0;
    virtual int   RayIntersection(Vec3* pDir, Vec3* pOrigin, Vec3* pHitOut) = 0;
    virtual int   SegmentIntersection(Vec3* pFrom, Vec3* pTo, Vec3* pOut)   = 0;
    // Variadic, so one address answers both the slot call and the six-argument call.
    virtual TransformObject* GetOrCreateTransform(...); // 0x0042e5a0
    virtual TransformObject* GetTransform();            // 0x00405820
    // unsigned char: the plane's body answers in AL alone.
    virtual unsigned char PointInside(const Vec3* pPoint)      = 0;
    virtual int           PointNearSurface(const Vec3* pPoint) = 0;
    virtual Vec3*         NormalAtPoint(const Vec3* pPoint)    = 0;
    virtual Vec3*         GetNormal(); // 0x00405870
    virtual float         RadiusForDirection(const Vec3* pVDir, int nUnused) = 0;
    virtual float         GetElasticity(); // 0x00405830
    virtual float         GetFriction();   // 0x00405840

    // The 0x04 to 0x38 run CollisionPlane and CollisionEllipsoid share.
    ShapeKind GetShapeKind() const
    {
        return m_nShapeKind;
    }

protected:
    // The 0x04 to 0x38 run CollisionPlane and CollisionEllipsoid share.
    int  m_nUnusedA; // 0x004
    int  m_nUnusedB; // 0x008
    Vec3 m_vNormal;  // 0x00c
    // Never set; the original's destructor frees it.
    void* m_pUnusedA; // 0x018
    short m_wUnusedA; // 0x01c
    short m_wUnusedB; // 0x01e
    // Never set; the original's destructor frees it.
    void* m_pUnusedB; // 0x020
    // GetOrCreateTransform stores it, GetTransform returns it.
    std::unique_ptr<TransformObject> m_pTransform; // 0x024
    // Never set; the original's destructor releases it through the pool's slot one.
    class PooledOwnerRoot* m_pPooled; // 0x028
    // The surface's half of the bounce, paired with the body's own flFriction and flElasticity.
    float     m_flFriction;   // 0x02c
    float     m_flElasticity; // 0x030
    ShapeKind m_nShapeKind;   // 0x034
};

// FUNCTION: HYPERX 0x0042e540
// CollisionShape::~CollisionShape

// The original's slot zero, which deletes the shape; nothing here calls it.
// FUNCTION: HYPERX 0x00405800
// CollisionShape::Destroy

// SYNTHETIC: HYPERX 0x00480850
// CollisionShape::operator=

#endif
