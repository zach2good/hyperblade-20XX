#include "common/compat.h"
#include "devicector.h"
#include "rampobject.h"
#include "physics/physicsbody.h"
#include "graphics/meshbone.h"
#include "gameengine.h"
#include "globals.h"
#include "engineglobals.h"
#include "graphics/meshboundingbox.h"
#include "math/transformobject.h"
#include "math/vector.h"
#include "physics/collisionobj.h"
#include "physics/collisionplane.h"
#include "engine/engineobject.h"

// Each of the four collision parts takes five copies of one global.

// FUNCTION: HYPERX 0x0047fc90
RampObject::RampObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    m_nFlags |= ENGINEOBJECT_NO_CULL;

    m_bPlanesReady = 0;
}

// Four planes through six corners of the bounding box, marked ready once built.
// FUNCTION: HYPERX 0x0047ff00
void RampObject::InitCollisionPlanes()
{
    Vec3            vMin;
    Vec3            vMax;
    Vec3            vNearMin;
    Vec3            vTipMin;
    Vec3            vTipMax;
    Vec3            vFarMin;
    Vec3            vFarMax;
    Vec3            vNearMax;
    MeshBoundingBox box;
    float           flFriction;
    float           flElasticity;

    m_bPlanesReady = 1;

    vMin = m_pBones->GetBoundingBox(&box)->vMin;
    vMax = m_pBones->GetBoundingBox(&box)->vMax;

    vTipMax.x = vMax.x * g_flOneThird;
    vTipMax.y = vMax.y;
    vTipMax.z = 0;

    vTipMin.x = vMin.x * g_flOneThird;
    vTipMin.y = vMax.y;
    vTipMin.z = 0;

    vNearMin = vMin;

    vFarMin.x = vMin.x;
    vFarMin.y = vMin.y;
    vFarMin.z = vMax.z;

    vFarMax.x = vMax.x;
    vFarMax.y = vMin.y;
    vFarMax.z = vMax.z;

    vNearMax.x = vMax.x;
    vNearMax.y = vMin.y;
    vNearMax.z = vMin.z;

    const float flNegMinZ = -vMin.z;

    RampEdge edge;

    edge.flSlope  = (vTipMax.x - vMax.x) / flNegMinZ;
    edge.flBase   = vMax.x - edge.flSlope * vMin.z;
    m_edgeNearMax = edge;

    const float flMaxZ = vMax.z;

    edge.flSlope = (vMax.x - vTipMax.x) / flMaxZ;
    edge.flBase  = vTipMax.x;
    m_edgeFarMax = edge;

    edge.flSlope  = (vTipMin.x - vMin.x) / flNegMinZ;
    edge.flBase   = vMin.x - edge.flSlope * vMin.z;
    m_edgeNearMin = edge;

    edge.flSlope = (vMin.x - vTipMin.x) / flMaxZ;
    edge.flBase  = vTipMin.x;
    m_edgeFarMin = edge;

    flFriction   = GetPhysicsBody()->flFriction;
    flElasticity = GetPhysicsBody()->flElasticity;

    {
        Vec3 vTipMaxA;
        Vec3 vFarMaxA;
        Vec3 vNearMaxA;

        m_transformC.TransformPoint(vNearMax, &vNearMaxA);
        m_transformC.TransformPoint(vFarMax, &vFarMaxA);
        m_transformC.TransformPoint(vTipMax, &vTipMaxA);

        CollisionObj planeMaxX(vTipMaxA, vFarMaxA, vNearMaxA, flFriction, flElasticity);

        m_planeMaxX.CopyInline(planeMaxX);
    }

    {
        Vec3 vTipMinB;
        Vec3 vNearMinB;
        Vec3 vFarMinB;

        m_transformC.TransformPoint(vFarMin, &vFarMinB);
        m_transformC.TransformPoint(vNearMin, &vNearMinB);
        m_transformC.TransformPoint(vTipMin, &vTipMinB);

        CollisionObj planeMinX(vTipMinB, vNearMinB, vFarMinB, flFriction, flElasticity);

        m_planeMinX.CopyInline(planeMinX);
    }

    {
        Vec3 vTipMaxC;
        Vec3 vNearMaxC;
        Vec3 vNearMinC;

        m_transformC.TransformPoint(vNearMin, &vNearMinC);
        m_transformC.TransformPoint(vNearMax, &vNearMaxC);
        m_transformC.TransformPoint(vTipMax, &vTipMaxC);

        CollisionObj planeNear(vTipMaxC, vNearMaxC, vNearMinC, flFriction, flElasticity);

        m_planeNear.CopyInline(planeNear);
    }

    {
        Vec3 vTipMinD;
        Vec3 vFarMinD;
        Vec3 vFarMaxD;

        m_transformC.TransformPoint(vFarMax, &vFarMaxD);
        m_transformC.TransformPoint(vFarMin, &vFarMinD);
        m_transformC.TransformPoint(vTipMin, &vTipMinD);

        CollisionObj planeFar(&vTipMinD, &vFarMinD, &vFarMaxD, flFriction, flElasticity);

        m_planeFar = planeFar;
    }
}

// Builds the planes on first use.
// FUNCTION: HYPERX 0x004808c0
CollisionObj* RampObject::GetCollisionObjForPoint(Vec3* pPoint)
{
    Vec3 vLocal;
    Vec3 vDelta;

    vLocal.x = m_transformC.m_a[0][3];
    vLocal.y = m_transformC.m_a[1][3];
    vLocal.z = m_transformC.m_a[2][3];

    vDelta.x = pPoint->x - vLocal.x;
    vDelta.y = pPoint->y - vLocal.y;
    vDelta.z = pPoint->z - vLocal.z;

    vLocal = vDelta;

    m_transformC.TransformDirectionByRows(vDelta, &vLocal);

    if (vLocal.x - (vLocal.z * m_edgeNearMax.flSlope + m_edgeNearMax.flBase) >= 0.0f && vLocal.x - (vLocal.z * m_edgeFarMax.flSlope + m_edgeFarMax.flBase) >= 0.0f)
    {
        if (m_bPlanesReady == 0)
        {
            InitCollisionPlanes();
        }

        return &m_planeMaxX;
    }

    if (vLocal.x - (vLocal.z * m_edgeNearMin.flSlope + m_edgeNearMin.flBase) < 0.0f && vLocal.x - (vLocal.z * m_edgeFarMin.flSlope + m_edgeFarMin.flBase) < 0.0f)
    {
        if (m_bPlanesReady == 0)
        {
            InitCollisionPlanes();
        }

        return &m_planeMinX;
    }

    if (vLocal.z < 0.0f)
    {
        if (m_bPlanesReady == 0)
        {
            InitCollisionPlanes();
        }

        return &m_planeNear;
    }

    if (m_bPlanesReady == 0)
    {
        InitCollisionPlanes();
    }

    return &m_planeFar;
}
