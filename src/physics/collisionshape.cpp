#include "common/compat.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "app/gameapp.h"
#include "collisionobj.h"
#include "common/pooledownerroot.h"
#include "math/transformobject.h"
#include "physics/collisionshape.h"

// FUNCTION: HYPERX 0x00405800
int CollisionShape::Destroy()
{
    delete this;

    return 0;
}

// FUNCTION: HYPERX 0x00405820
TransformObject* CollisionShape::GetTransform()
{
    return m_pTransform;
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
, m_pUnusedA(0)
, m_wUnusedA(0)
, m_wUnusedB(0)
, m_pUnusedB(0)
, m_pTransform(0)
, m_pPooled(0)
, m_nShapeKind(SHAPE_KIND_PLANE)
{
    m_vNormal.x = m_vNormal.y = m_vNormal.z = g_flZeroCollisionObj;
}

// FUNCTION: HYPERX 0x0042e540
CollisionShape::~CollisionShape()
{
    if (m_pUnusedA != 0)
    {
        operator delete(m_pUnusedA);

        m_pUnusedA = 0;
    }

    if (m_pUnusedB != 0)
    {
        operator delete(m_pUnusedB);

        m_pUnusedB = 0;
    }

    if (m_pTransform != 0)
    {
        m_pTransform = m_pTransform->DestroyViaVtable();
    }

    if (m_pPooled != 0)
    {
        m_pPooled = m_pPooled->Destroy();
    }
}

// The plane's slot thirteen calls this with six arguments, so it is variadic and __cdecl.
// An existing transform is reset through its own table; a failed allocation stores null.
// FUNCTION: HYPERX 0x0042e5a0
TransformObject* CollisionShape::GetOrCreateTransform(...)
{
    if (m_pTransform == 0)
    {
        m_pTransform = new TransformObject;
    }
    else
    {
        m_pTransform->ResetToIdentity();
    }

    return m_pTransform;
}
