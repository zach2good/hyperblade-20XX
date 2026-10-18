#include "common/compat.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "common/staticinit.h"
#include "collisionplane.h"
#include "collisionshape.h"
#include <cmath>
#include "physicsglobals.h"

// FUNCTION: HYPERX 0x00405860
Vec3* CollisionPlane::NormalAtPoint(const Vec3*)
{
    return &m_vNormal;
}

// Handing six values to the transform getter, which reads none of them.
// FUNCTION: HYPERX 0x00405880
void CollisionPlane::ForwardTransformRequest(int nA, int nB, float flC, float flD, float flE, float flF)
{
    CollisionShape::GetOrCreateTransform(nA, nB, flC, flE, flD, flF);
}

// FUNCTION: HYPERX 0x004058c0
unsigned char CollisionPlane::PointInside(const Vec3*)
{
    return 0x1;
}

// On the surface when the point's height is within a hundredth of the surface's.
// FUNCTION: HYPERX 0x004058d0
int CollisionPlane::PointNearSurface(const Vec3* pPoint)
{
    const float flHeight = GetZ(pPoint->x, pPoint->y);

    if (fabs(static_cast<double>(flHeight - pPoint->z)) < g_dSurfaceTolerance)
    {
        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00405910
float CollisionPlane::RadiusForDirection(const Vec3*, int)
{
    return -g_flOneCollisionPlane;
}

// The direction lives in a file-scope vector, so two of these cannot run at once.
// FUNCTION: HYPERX 0x0042e7d0
int CollisionPlane::SegmentIntersection(Vec3* pFrom, Vec3* pTo, Vec3* pOut)
{
    g_vPlaneSegmentDir.x = pTo->x - pFrom->x;
    g_vPlaneSegmentDir.y = pTo->y - pFrom->y;
    g_vPlaneSegmentDir.z = pTo->z - pFrom->z;

    return RayIntersection(&g_vPlaneSegmentDir, pFrom, pOut);
}

// Tests the whole line: answers -1 for a null direction, 0 when parallel, 1 with the crossing.
// FUNCTION: HYPERX 0x0042e810
int CollisionPlane::RayIntersection(Vec3* pVDir, Vec3* pVOrigin, Vec3* pVHitOut)
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

// FUNCTION: HYPERX 0x0042e930
float CollisionPlane::GetZ(float x, float y)
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
