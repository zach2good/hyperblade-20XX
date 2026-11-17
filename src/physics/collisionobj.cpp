#include "common/compat.h"
#include <cstdint>
#include "collisionobj.h"
#include "app/gameapp.h"
#include "globals.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/modelids.h"
#include "engine/collisionobjectids.h"
#include "app/stringids.h"
#include "common/staticinit.h"
#include <cmath>
#include "physicsglobals.h"

float g_flUnusedPlanA = g_flZeroPlan;
float g_flUnusedPlanB = g_flZeroPlan;
float g_flUnusedPlanC = g_flZeroPlan;

// FUNCTION: HYPERX1001 0x00405860
Vec3* CollisionObj::NormalAtPoint(const Vec3*)
{
    return &m_vNormal;
}

// Handing six values to the transform getter, which reads none of them.
// FUNCTION: HYPERX1001 0x00405880
void CollisionObj::ForwardTransformRequest(int nA, int nB, float flC, float flD, float flE, float flF)
{
    CollisionShape::GetOrCreateTransform(nA, nB, flC, flE, flD, flF);
}

// FUNCTION: HYPERX1001 0x004058c0
unsigned char CollisionObj::PointInside(const Vec3*)
{
    return 0x1;
}

// On the surface when the point's height is within a hundredth of the surface's.
// FUNCTION: HYPERX1001 0x004058d0
int CollisionObj::PointNearSurface(const Vec3* pPoint)
{
    const float flHeight = GetZ(pPoint->x, pPoint->y);

    if (fabs(static_cast<double>(flHeight - pPoint->z)) < g_dSurfaceTolerance)
    {
        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX1001 0x00405910
float CollisionObj::RadiusForDirection(const Vec3*, int)
{
    return -g_flOneCollisionObj;
}

// FUNCTION: HYPERX1001 0x0042e640
CollisionObj::CollisionObj(const Vec3* pNormal, const Vec3* pOrigin, float flFriction, float flElasticity)
{
    m_vNormal.x = pNormal->x;
    m_vNormal.y = pNormal->y;
    m_vNormal.z = pNormal->z;

    if (m_vNormal.x == g_flZeroCollisionObj && m_vNormal.y == g_flZeroCollisionObj && m_vNormal.z == g_flZeroCollisionObj)
    {
        g_gameApp.FatalString((const char*)static_cast<std::uintptr_t>(static_cast<unsigned short>(kStringZeroNormalInPlane)));
    }

    m_vPoint = *pOrigin;

    m_nShapeKind   = ShapeKind::kPlane;
    m_flFriction   = flFriction;
    m_flElasticity = flElasticity;
    m_bCached      = 0;
}

// The failure path passes a string resource id where a format string belongs.
// FUNCTION: HYPERX1001 0x0042e710
void CollisionObj::InitFrom3Points(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity)
{
    const auto flUx = pB->x - pOrigin->x;
    const auto flUy = pB->y - pOrigin->y;
    const auto flUz = pB->z - pOrigin->z;
    // The client writes all three components through ECX (0x0042e75a to 0x0042e77b).
    auto* const pNormal = &m_vNormal;
    Vec3        vV;

    vV.x = pC->x - pOrigin->x;
    vV.y = pC->y - pOrigin->y;
    vV.z = pC->z - pOrigin->z;

    pNormal->x = flUy * vV.z - flUz * vV.y;
    pNormal->y = flUz * vV.x - flUx * vV.z;
    pNormal->z = flUx * vV.y - flUy * vV.x;

    if (!Math::Vec3Normalize(pNormal))
    {
        g_gameApp.FatalString((const char*)static_cast<std::uintptr_t>(static_cast<unsigned short>(kStringZeroNormalInPlane)));
    }

    m_vPoint = *pOrigin;

    m_nShapeKind   = ShapeKind::kPlane;
    m_flFriction   = flFriction;
    m_flElasticity = flElasticity;
}

// The direction lives in a file-scope vector, so two of these cannot run at once.
// FUNCTION: HYPERX1001 0x0042e7d0
int CollisionObj::SegmentIntersection(Vec3* pFrom, Vec3* pTo, Vec3* pOut)
{
    g_vPlaneSegmentDir.x = pTo->x - pFrom->x;
    g_vPlaneSegmentDir.y = pTo->y - pFrom->y;
    g_vPlaneSegmentDir.z = pTo->z - pFrom->z;

    return RayIntersection(&g_vPlaneSegmentDir, pFrom, pOut);
}

// Tests the whole line: answers -1 for a null direction, 0 when parallel, 1 with the crossing.
// FUNCTION: HYPERX1001 0x0042e810
int CollisionObj::RayIntersection(Vec3* pVDir, Vec3* pVOrigin, Vec3* pVHitOut)
{
    Vec3              vScaled;
    Vec3              vHit;
    const Vec3* const pNormal = &m_vNormal;
    Vec3              vDelta;
    float             flDenominator;
    float             flDistance;

    if (pVDir->x < g_dNormalizeEpsilon && pVDir->x > -1e-07 && pVDir->y < g_dNormalizeEpsilon && pVDir->y > -1e-07 && pVDir->z < g_dNormalizeEpsilon && pVDir->z > -1e-07)
    {
        return -1;
    }

    if ((flDenominator = m_vNormal.x * pVDir->x + (pNormal->y * pVDir->y + pNormal->z * pVDir->z)) == g_flZeroCollisionObj)
    {
        return 0;
    }

    vDelta.x = m_vPoint.x - pVOrigin->x;
    vDelta.y = m_vPoint.y - pVOrigin->y;
    vDelta.z = m_vPoint.z - pVOrigin->z;

    flDistance = (vDelta.x * pNormal->x + (vDelta.y * pNormal->y + vDelta.z * pNormal->z)) / flDenominator;

    vScaled.x = flDistance * pVDir->x;
    vScaled.y = flDistance * pVDir->y;
    vScaled.z = flDistance * pVDir->z;

    vHit.x = vScaled.x + pVOrigin->x;
    vHit.y = vScaled.y + pVOrigin->y;
    vHit.z = vScaled.z + pVOrigin->z;

    *pVHitOut = vHit;

    return 1;
}

// FUNCTION: HYPERX1001 0x0042e930
float CollisionObj::GetZ(float x, float y)
{
    if (m_bCached == 0)
    {
        if (m_vNormal.z == g_flZeroCollisionObj)
        {
            m_flOffset = g_flZeroCollisionObj;
            m_flSlopeY = g_flZeroCollisionObj;
            m_flSlopeX = g_flZeroCollisionObj;
        }
        else
        {
            m_flSlopeX = -(m_vNormal.x / m_vNormal.z);

            m_flSlopeY = -(m_vNormal.y / m_vNormal.z);

            const Vec3* const pPoint  = &m_vPoint;
            const Vec3* const pNormal = &m_vNormal;

            m_flOffset =
                (pNormal->x * pPoint->x + pNormal->y * pPoint->y + pNormal->z * pPoint->z) / m_vNormal.z;
        }

        m_bCached = 1;
    }

    return x * m_flSlopeX + y * m_flSlopeY + m_flOffset;
}

// Maps a CollisionObjectID to its ModelType; an unknown id is fatal.
// FUNCTION: HYPERX1001 0x0045e200
int CollisionObj::GetModelId(int nIndex)
{
    switch (nIndex)
    {
        case kCOBDromer0:
        case kCOBBatonEzone0:
        case kCOBDromerBody0:
        case kCOBDr0Awareness:
            return kSlotDromer0;

        case kCOBDromer1:
        case kCOBBatonEzone1:
        case kCOBDromerBody1:
        case kCOBDr1Awareness:
            return kSlotDromer1;

        case kCOBDromer2:
        case kCOBBatonEzone2:
        case kCOBDromerBody2:
        case kCOBDr2Awareness:
            return kSlotDromer2;

        case kCOBDromer3:
        case kCOBBatonEzone3:
        case kCOBDromerBody3:
        case kCOBDr3Awareness:
            return kSlotDromer3;

        case kCOBDromer4:
        case kCOBBatonEzone4:
        case kCOBDromerBody4:
        case kCOBDr4Awareness:
            return kGoalieSlotA;

        case kCOBDromer5:
        case kCOBBatonEzone5:
        case kCOBDromerBody5:
        case kCOBDr5Awareness:
            return kGoalieSlotB;

        case kCOBNgoalZone:
        case kCOBNgoalCase:
        case kCOBNgoalPort:
        case kCOBNgoalSide:
            return kSlotNgoal;

        case kCOBSgoalZone:
        case kCOBSgoalCase:
        case kCOBSgoalPort:
        case kCOBSgoalSide:
            return kSlotSgoal;

        case kCOBMultiplier:
            return kSlotMultiplier;

        case kCOBMine:
            return kSlotMine;

        case kCOBSwarf:
            return kSlotSwarf;

        default:
            // STRING: HYPERX1001 0x004c45cc
            g_gameApp.Fatal("ColObj to Model map unaccounted for.\n");

            return kModelIDNone;
    }
}

// FUNCTION: HYPERX1001 0x00480740
CollisionObj::CollisionObj(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity)
{
    InitFrom3Points(pOrigin, pB, pC, flFriction, flElasticity);
}
