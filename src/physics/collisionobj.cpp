#include "common/compat.h"
#include "collisionobj.h"
#include "app/gameapp.h"
#include "globals.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/modelids.h"
#include "engine/collisionobjectids.h"
#include "app/stringids.h"
#include "common/staticinit.h"
#include <math.h>
#include "physicsglobals.h"

float g_flUnusedPlanA = g_flZeroPlan;
float g_flUnusedPlanB = g_flZeroPlan;
float g_flUnusedPlanC = g_flZeroPlan;

// FUNCTION: HYPERX1000 0x004122e0
// FUNCTION: HYPERX1001 0x00405860
Vec3* CollisionObj::NormalAtPoint(const Vec3*)
{
    return &m_vNormal;
}

// Handing six values to the transform getter, which reads none of them.
// FUNCTION: HYPERX1000 0x00412300
// FUNCTION: HYPERX1001 0x00405880
void CollisionObj::ForwardTransformRequest(int nA, int nB, float flC, float flD, float flE, float flF)
{
    CollisionShape::GetOrCreateTransform(nA, nB, flC, flE, flD, flF);
}

// FUNCTION: HYPERX1000 0x00412360
// FUNCTION: HYPERX1001 0x004058c0
unsigned char CollisionObj::PointInside(const Vec3*)
{
    return 0x1;
}

// On the surface when the point's height is within a hundredth of the surface's.
// FUNCTION: HYPERX1000 0x00412370
// FUNCTION: HYPERX1001 0x004058d0
int CollisionObj::PointNearSurface(const Vec3* pPoint)
{
    const float flHeight = GetZ(pPoint->x, pPoint->y);

    if (fabs(flHeight - pPoint->z) < g_dSurfaceTolerance)
    {
        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX1000 0x004123b0
// FUNCTION: HYPERX1001 0x00405910
float CollisionObj::RadiusForDirection(const Vec3*, int)
{
    return -g_flOneCollisionObj;
}

// FUNCTION: HYPERX1000 0x00494e20
// FUNCTION: HYPERX1001 0x0042e640
CollisionObj::CollisionObj(const Vec3* pNormal, const Vec3* pOrigin, float flFriction, float flElasticity)
{
    m_vNormal.x = pNormal->x;
    m_vNormal.y = pNormal->y;
    m_vNormal.z = pNormal->z;

    if (m_vNormal.x == g_flZeroCollisionObj && m_vNormal.y == g_flZeroCollisionObj && m_vNormal.z == g_flZeroCollisionObj)
    {
        g_gameApp.FatalString(MAKEINTRESOURCEA(STRING_ZERO_NORMAL_IN_PLANE));
    }

    m_vPoint = *pOrigin;

    m_nShapeKind   = SHAPE_KIND_PLANE;
    m_flFriction   = flFriction;
    m_flElasticity = flElasticity;
    m_bCached      = 0;
}

// The failure path passes a string resource id where a format string belongs.
// FUNCTION: HYPERX1000 0x00495010
// FUNCTION: HYPERX1001 0x0042e710
void CollisionObj::InitFrom3Points(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity)
{
    const float flUx = pB->x - pOrigin->x;
    const float flUy = pB->y - pOrigin->y;
    const float flUz = pB->z - pOrigin->z;
    // The client writes all three components through ECX (0x0042e75a to 0x0042e77b).
    Vec3* const pNormal = &m_vNormal;
    Vec3        vV;

    vV.x = pC->x - pOrigin->x;
    vV.y = pC->y - pOrigin->y;
    vV.z = pC->z - pOrigin->z;

    pNormal->x = flUy * vV.z - flUz * vV.y;
    pNormal->y = flUz * vV.x - flUx * vV.z;
    pNormal->z = flUx * vV.y - flUy * vV.x;

    if (!Math::Vec3Normalize(pNormal))
    {
        g_gameApp.FatalString(MAKEINTRESOURCEA(STRING_ZERO_NORMAL_IN_PLANE));
    }

    m_vPoint = *pOrigin;

    m_nShapeKind   = SHAPE_KIND_PLANE;
    m_flFriction   = flFriction;
    m_flElasticity = flElasticity;
}

// The direction lives in a file-scope vector, so two of these cannot run at once.
// FUNCTION: HYPERX1000 0x00495130
// FUNCTION: HYPERX1001 0x0042e7d0
int CollisionObj::SegmentIntersection(Vec3* pFrom, Vec3* pTo, Vec3* pOut)
{
    g_vPlaneSegmentDir.x = pTo->x - pFrom->x;
    g_vPlaneSegmentDir.y = pTo->y - pFrom->y;
    g_vPlaneSegmentDir.z = pTo->z - pFrom->z;

    return RayIntersection(&g_vPlaneSegmentDir, pFrom, pOut);
}

// Tests the whole line: answers -1 for a null direction, 0 when parallel, 1 with the crossing.
// FUNCTION: HYPERX1000 0x00495180
// FUNCTION: HYPERX1001 0x0042e810
int CollisionObj::RayIntersection(Vec3* pVDir, Vec3* pVOrigin, Vec3* pVHitOut)
{
    Vec3        vScaled;
    Vec3        vHit;
    const Vec3* pNormal = &m_vNormal;
    Vec3        vDelta;
    float       flDenominator;
    float       flDistance;

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

// FUNCTION: HYPERX1000 0x00495320
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

            const Vec3* pPoint  = &m_vPoint;
            const Vec3* pNormal = &m_vNormal;

            m_flOffset =
                (pNormal->x * pPoint->x + pNormal->y * pPoint->y + pNormal->z * pPoint->z) / m_vNormal.z;
        }

        m_bCached = 1;
    }

    return x * m_flSlopeX + y * m_flSlopeY + m_flOffset;
}

// Maps a CollisionObjectID to its ModelType; an unknown id is fatal.
// FUNCTION: HYPERX1000 0x004a5f80
// FUNCTION: HYPERX1001 0x0045e200
int __cdecl CollisionObj::GetModelId(int nIndex)
{
    switch (nIndex)
    {
        case COB_DROMER_0:
        case COB_BATON_EZONE_0:
        case COB_DROMER_BODY_0:
        case COB_DR0_AWARENESS:
            return SLOT_DROMER0;

        case COB_DROMER_1:
        case COB_BATON_EZONE_1:
        case COB_DROMER_BODY_1:
        case COB_DR1_AWARENESS:
            return SLOT_DROMER1;

        case COB_DROMER_2:
        case COB_BATON_EZONE_2:
        case COB_DROMER_BODY_2:
        case COB_DR2_AWARENESS:
            return SLOT_DROMER2;

        case COB_DROMER_3:
        case COB_BATON_EZONE_3:
        case COB_DROMER_BODY_3:
        case COB_DR3_AWARENESS:
            return SLOT_DROMER3;

        case COB_DROMER_4:
        case COB_BATON_EZONE_4:
        case COB_DROMER_BODY_4:
        case COB_DR4_AWARENESS:
            return GOALIE_SLOT_A;

        case COB_DROMER_5:
        case COB_BATON_EZONE_5:
        case COB_DROMER_BODY_5:
        case COB_DR5_AWARENESS:
            return GOALIE_SLOT_B;

        case COB_NGOAL_ZONE:
        case COB_NGOAL_CASE:
        case COB_NGOAL_PORT:
        case COB_NGOAL_SIDE:
            return SLOT_NGOAL;

        case COB_SGOAL_ZONE:
        case COB_SGOAL_CASE:
        case COB_SGOAL_PORT:
        case COB_SGOAL_SIDE:
            return SLOT_SGOAL;

        case COB_MULTIPLIER:
            return SLOT_MULTIPLIER;

        case COB_MINE:
            return SLOT_MINE;

        case COB_SWARF:
            return SLOT_SWARF;

        default:
            // STRING: HYPERX1001 0x004c45cc
            g_gameApp.Fatal("ColObj to Model map unaccounted for.\n");

            return MODEL_ID_NONE;
    }
}

// FUNCTION: HYPERX1001 0x00480740
CollisionObj::CollisionObj(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity)
{
    InitFrom3Points(pOrigin, pB, pC, flFriction, flElasticity);
}

// FUNCTION: HYPERX1000 0x0046ccf0
// FUNCTION: HYPERX1001 0x004807b0
CollisionObj& CollisionObj::operator=(const CollisionObj& other)
{
    m_nUnusedA     = other.m_nUnusedA;
    m_nUnusedB     = other.m_nUnusedB;
    m_vNormal      = other.m_vNormal;
    m_pUnusedA     = other.m_pUnusedA;
    m_wUnusedA     = other.m_wUnusedA;
    m_wUnusedB     = other.m_wUnusedB;
    m_pUnusedB     = other.m_pUnusedB;
    m_pTransform   = other.m_pTransform;
    m_pPooled      = other.m_pPooled;
    m_flFriction   = other.m_flFriction;
    m_flElasticity = other.m_flElasticity;
    m_nShapeKind   = other.m_nShapeKind;

    m_vPoint   = other.m_vPoint;
    m_flSlopeX = other.m_flSlopeX;
    m_flSlopeY = other.m_flSlopeY;
    m_flOffset = other.m_flOffset;
    m_bCached  = other.m_bCached;

    return *this;
}
