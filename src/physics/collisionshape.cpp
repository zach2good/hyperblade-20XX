#include "common/compat.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "app/gameapp.h"
#include "collisionobj.h"
#include "math/transformobject.h"
#include "physics/collisionshape.h"
#include <memory>

// FUNCTION: HYPERX 0x00405820
TransformObject* CollisionShape::GetTransform()
{
    return m_pTransform.get();
}

// FUNCTION: HYPERX 0x00405830
float CollisionShape::GetElasticity()
{
    return m_flElasticity;
}

// FUNCTION: HYPERX 0x00405840
float CollisionShape::GetFriction()
{
    return m_flFriction;
}

// FUNCTION: HYPERX 0x00405870
Vec3* CollisionShape::GetNormal()
{
    return &m_vNormal;
}

// FUNCTION: HYPERX 0x0042e4f0
CollisionShape::CollisionShape()
: m_nUnusedA(0)
, m_nUnusedB(0)
, m_pUnusedA(nullptr)
, m_wUnusedA(0)
, m_wUnusedB(0)
, m_pUnusedB(nullptr)
, m_pPooled(nullptr)
, m_nShapeKind(ShapeKind::kPlane)
{
    m_vNormal.x = m_vNormal.y = m_vNormal.z = g_flZeroCollisionObj;
}

// The plane's slot thirteen calls this with six arguments, so it is variadic and __cdecl.
// An existing transform is reset through its own table; a failed allocation stores null.
// FUNCTION: HYPERX 0x0042e5a0
TransformObject* CollisionShape::GetOrCreateTransform(...)
{
    if (m_pTransform == nullptr)
    {
        m_pTransform = std::make_unique<TransformObject>();
    }
    else
    {
        m_pTransform->ResetToIdentity();
    }

    return m_pTransform.get();
}
