#include "common/compat.h"
#include "collisionellipsoid.h"
#include "globals.h"
#include "physicsglobals.h"
#include "math/vector.h"
#include "common/stub.h"
#include "ui/hudglobals.h"
#include <cmath>

// FUNCTION: HYPERX 0x00408e90
CollisionEllipsoid::~CollisionEllipsoid() = default;

// FUNCTION: HYPERX 0x00408ea0
unsigned char CollisionEllipsoid::PointInside(const Vec3* pPoint)
{
    int nInside;

    const float flSum = m_flInvZZ * pPoint->z * pPoint->z + m_flInvYY * pPoint->y * pPoint->y + m_flInvXX * pPoint->x * pPoint->x;

    nInside = flSum <= 1.0;

    return static_cast<unsigned char>(nInside);
}

// FUNCTION: HYPERX 0x00408ee0
int CollisionEllipsoid::PointNearSurface(const Vec3* pPoint)
{
    const float flSum = m_flInvZZ * pPoint->z * pPoint->z + m_flInvYY * pPoint->y * pPoint->y + m_flInvXX * pPoint->x * pPoint->x;

    if (flSum <= 0.99f)
    {
        return 1;
    }

    return 0;
}

// The camera's shell passes friction 0.01 and elasticity 0.5; the arena's 0.01 and 0.95.
// FUNCTION: HYPERX 0x0042e9b0
CollisionEllipsoid::CollisionEllipsoid(float flX, float flY, float flZ, float flFriction, float flElasticity)
: m_flRadiusX(flX)
, m_flRadiusY(flY)
, m_flRadiusZ(flZ)
, m_flInvXX(g_flCollisionOne / (flX * flX))
, m_flInvYY(g_flCollisionOne / (flY * flY))
, m_flInvZZ(g_flCollisionOne / (flZ * flZ))
{
    m_flFriction   = flFriction;
    m_flElasticity = flElasticity;
    m_nShapeKind   = SHAPE_KIND_ELLIPSOID;
}

// The ray's origin is the second point, not the first; the plane's version uses the first.
// FUNCTION: HYPERX 0x0042ea30
int CollisionEllipsoid::SegmentIntersection(Vec3* pFirst, Vec3* pSecond, Vec3* pNearOutIn)
{
    Vec3 vDirection;
    Vec3 vFarUnused;

    vDirection.x = pSecond->x - pFirst->x;
    vDirection.y = pSecond->y - pFirst->y;
    vDirection.z = pSecond->z - pFirst->z;

    return IntersectRay(&vDirection, pSecond, pNearOutIn, &vFarUnused);
}

// FUNCTION: HYPERX 0x0042ea80
int CollisionEllipsoid::IntersectSegment(Vec3* pFirst, Vec3* pSecond, Vec3* pNearOut, Vec3* pFarOut)
{
    Vec3 vDirection;

    vDirection.x = pSecond->x - pFirst->x;
    vDirection.y = pSecond->y - pFirst->y;
    vDirection.z = pSecond->z - pFirst->z;

    return IntersectRay(&vDirection, pSecond, pNearOut, pFarOut);
}

// The far point goes into a scratch vector that is never read.

// FUNCTION: HYPERX 0x0042ead0
int CollisionEllipsoid::RayIntersection(Vec3* pDirection, Vec3* pOrigin, Vec3* pPointOut)
{
    Vec3 vFarUnused;

    return IntersectRay(pDirection, pOrigin, pPointOut, &vFarUnused);
}

// The band either side of nought a direction component must leave to parameterise the line.
#define AXIS_EPS_UPPER 0.0001
#define AXIS_EPS_LOWER -0.0001

// Parameterised by the first safe direction component, not the largest; solved in double.
// FUNCTION: HYPERX 0x0042eb00
int CollisionEllipsoid::IntersectRay(Vec3* pDirection, Vec3* pOrigin, Vec3* pNearOut, Vec3* pFarOut)
{
    double dSlopeA;
    double dSlopeB;
    double dQuad;
    double dHalfB;
    double dArmA;
    double dArmB;
    double dRoot;
    double dRootLo;
    double dRootHi;
    double dOffsetLo;
    double dOffsetHi;

    const double dOriginX = pOrigin->x;
    const double dOriginY = pOrigin->y;
    const double dOriginZ = pOrigin->z;

    const double dDirX = pDirection->x;
    double       dDirY = pDirection->y;
    const double dDirZ = pDirection->z;

    if (dDirX > AXIS_EPS_UPPER || dDirX < AXIS_EPS_LOWER)
    {
        dSlopeA = dDirY / dDirX;
        dSlopeB = dDirZ / dDirX;

        dQuad = (m_flInvYY * (dSlopeA * dSlopeA)) + m_flInvZZ * (dSlopeB * dSlopeB) + m_flInvXX;

        dHalfB = (m_flInvYY * (dSlopeA * dOriginY - dSlopeA * dSlopeA * dOriginX) + (dSlopeB * dOriginZ - dSlopeB * dSlopeB * dOriginX) * m_flInvZZ) / dQuad;

        dArmA = dSlopeA * dOriginX - dOriginY;
        dArmB = dSlopeB * dOriginX - dOriginZ;

        if (g_flZeroCollisionObj <= (dRoot = dHalfB * dHalfB - ((m_flInvZZ * dArmB * dArmB + m_flInvYY * dArmA * dArmA) - g_flCollisionOne) / dQuad))
        {
            dRoot = sqrt(static_cast<double>(dRoot));
        }
        else if (dRoot > AXIS_EPS_LOWER)
        {
            dRoot = 0.0;
        }
        else
        {
            return 0;
        }

        dRootLo = -dHalfB - dRoot;
        dRootHi = dRoot - dHalfB;

        dOffsetLo = dRootLo - dOriginX;
        dOffsetHi = dRootHi - dOriginX;

        if (fabs(static_cast<double>(dOffsetLo)) <= fabs(static_cast<double>(dOffsetHi)))
        {
            pNearOut->x = static_cast<float>(dRootLo);
            pNearOut->y = static_cast<float>(dOffsetLo * dSlopeA + dOriginY);
            pNearOut->z = static_cast<float>(dOffsetLo * dSlopeB + dOriginZ);

            pFarOut->x = static_cast<float>(dRootHi);
            pFarOut->y = static_cast<float>(dOffsetHi * dSlopeA + dOriginY);
            pFarOut->z = static_cast<float>(dOffsetHi * dSlopeB + dOriginZ);

            Vec3::FindClosest(pOrigin, pNearOut, pFarOut);

            return 1;
        }
        else
        {
            pNearOut->x = static_cast<float>(dRootHi);
            pNearOut->y = static_cast<float>(dOffsetHi * dSlopeA + dOriginY);
            pNearOut->z = static_cast<float>(dOffsetHi * dSlopeB + dOriginZ);

            pFarOut->x = static_cast<float>(dRootLo);
            pFarOut->y = static_cast<float>(dOffsetLo * dSlopeA + dOriginY);
            pFarOut->z = static_cast<float>(dOffsetLo * dSlopeB + dOriginZ);

            Vec3::FindClosest(pOrigin, pNearOut, pFarOut);

            return 1;
        }
    }
    else if (dDirY <= AXIS_EPS_UPPER && dDirY >= AXIS_EPS_LOWER)
    {
        dSlopeA = dDirY / dDirZ;
        dSlopeB = dDirX / dDirZ;

        dQuad = (m_flInvYY * (dSlopeA * dSlopeA)) + m_flInvXX * (dSlopeB * dSlopeB) + m_flInvZZ;

        dHalfB = ((dSlopeB * dOriginX - dSlopeB * dSlopeB * dOriginZ) * m_flInvXX + (m_flInvYY * (dSlopeA * dOriginY - dSlopeA * dSlopeA * dOriginZ))) / dQuad;

        dArmA = dSlopeA * dOriginZ - dOriginY;
        dArmB = dSlopeB * dOriginZ - dOriginX;

        if (g_flZeroCollisionObj <= (dRoot = dHalfB * dHalfB - ((m_flInvXX * dArmB * dArmB + (m_flInvYY * dArmA * dArmA)) - g_flCollisionOne) / dQuad))
        {
            dRoot = sqrt(static_cast<double>(dRoot));
        }
        else if (dRoot > AXIS_EPS_LOWER)
        {
            dRoot = g_flZeroCollisionObj;
        }
        else
        {
            return 0;
        }

        dRootLo = -dHalfB - dRoot;
        dRootHi = dRoot - dHalfB;

        dOffsetLo = dRootLo - dOriginZ;
        dOffsetHi = dRootHi - dOriginZ;

        if (fabs(static_cast<double>(dOffsetLo)) <= fabs(static_cast<double>(dOffsetHi)))
        {
            pNearOut->z = static_cast<float>(dRootLo);
            pNearOut->y = static_cast<float>(dOffsetLo * dSlopeA + dOriginY);
            pNearOut->x = static_cast<float>(dSlopeB * dOffsetLo + dOriginX);

            pFarOut->z = static_cast<float>(dRootHi);
            pFarOut->y = static_cast<float>(dOffsetHi * dSlopeA + dOriginY);
            pFarOut->x = static_cast<float>(dSlopeB * dOffsetHi + dOriginX);

            Vec3::FindClosest(pOrigin, pNearOut, pFarOut);

            return 1;
        }
        else
        {
            pNearOut->z = static_cast<float>(dRootHi);
            pNearOut->y = static_cast<float>(dOffsetHi * dSlopeA + dOriginY);
            pNearOut->x = static_cast<float>(dSlopeB * dOffsetHi + dOriginX);

            pFarOut->z = static_cast<float>(dRootLo);
            pFarOut->y = static_cast<float>(dOffsetLo * dSlopeA + dOriginY);
            pFarOut->x = static_cast<float>(dSlopeB * dOffsetLo + dOriginX);

            Vec3::FindClosest(pOrigin, pNearOut, pFarOut);

            return 1;
        }
    }
    else
    {
        dSlopeB = dDirX / dDirY;
        dSlopeA = dDirZ / dDirY;

        dQuad = m_flInvXX * (dSlopeB * dSlopeB) + m_flInvZZ * (dSlopeA * dSlopeA) + m_flInvYY;

        dHalfB = ((dSlopeB * dOriginX - dSlopeB * dSlopeB * dOriginY) * m_flInvXX + (m_flInvZZ * (dSlopeA * dOriginZ - dSlopeA * dSlopeA * dOriginY))) / dQuad;

        dArmB = dSlopeB * dOriginY - dOriginX;
        dArmA = dSlopeA * dOriginY - dOriginZ;

        if (g_flZeroCollisionObj <= (dRoot = dHalfB * dHalfB - ((m_flInvZZ * dArmA * dArmA + m_flInvXX * dArmB * dArmB) - g_flCollisionOne) / dQuad))
        {
            dRoot = sqrt(static_cast<double>(dRoot));
        }
        else if (dRoot > AXIS_EPS_LOWER)
        {
            dRoot = g_flZeroCollisionObj;
        }
        else
        {
            return 0;
        }

        dRootLo = -dHalfB - dRoot;
        dRootHi = dRoot - dHalfB;

        dOffsetLo = dRootLo - dOriginY;
        dOffsetHi = dRootHi - dOriginY;

        if (fabs(static_cast<double>(dOffsetLo)) <= fabs(static_cast<double>(dOffsetHi)))
        {
            pNearOut->y = static_cast<float>(dRootLo);
            pNearOut->x = static_cast<float>(dSlopeB * dOffsetLo + dOriginX);
            pNearOut->z = static_cast<float>(dOffsetLo * dSlopeA + dOriginZ);

            pFarOut->y = static_cast<float>(dRootHi);
            pFarOut->x = static_cast<float>(dSlopeB * dOffsetHi + dOriginX);
            pFarOut->z = static_cast<float>(dOffsetHi * dSlopeA + dOriginZ);

            Vec3::FindClosest(pOrigin, pNearOut, pFarOut);

            return 1;
        }
        else
        {
            pNearOut->y = static_cast<float>(dRootHi);
            pNearOut->x = static_cast<float>(dSlopeB * dOffsetHi + dOriginX);
            pNearOut->z = static_cast<float>(dOffsetHi * dSlopeA + dOriginZ);

            pFarOut->y = static_cast<float>(dRootLo);
            pFarOut->x = static_cast<float>(dSlopeB * dOffsetLo + dOriginX);
            pFarOut->z = static_cast<float>(dOffsetLo * dSlopeA + dOriginZ);

            Vec3::FindClosest(pOrigin, pNearOut, pFarOut);

            return 1;
        }
    }
}

// The epsilon appears twice at two addresses: as a double and as a float.
#define AXIS_EPS        1e-05
#define AXIS_EPS_SQUARE 1e-05f

// Wrong near the axis: with |x| under 1e-5 it answers about (-a, 0, z) instead of (0, +/-b, z).
// FUNCTION: HYPERX 0x0042f040
void CollisionEllipsoid::ComputeRadialBoundaryXY(Vec3* pIn, Vec3* pOut)
{
    char  bOffAxis = 1;
    float flSquare;
    float flRadius;

    if (fabs(static_cast<double>(pIn->x)) > AXIS_EPS)
    {
        flSquare = pIn->x * pIn->x;
    }
    else
    {
        flSquare = AXIS_EPS_SQUARE;
        bOffAxis = 0;
    }

    flRadius = static_cast<float>(sqrt(static_cast<double>(flSquare / (flSquare * m_flInvXX + m_flInvYY * pIn->y * pIn->y))));

    if (pIn->x <= 0.0)
    {
        flRadius = -flRadius;
    }

    pOut->x = flRadius;

    if (bOffAxis)
    {
        pOut->y = (pIn->y / pIn->x) * flRadius;
        pOut->z = pIn->z;
        return;
    }

    pOut->y = g_flZeroCollisionObj;
    pOut->z = pIn->z;
}

// A point outside the ellipse answers one, the value the solve started from.

// FUNCTION: HYPERX 0x0042f0d0
float CollisionEllipsoid::GetZ(float flX, float flY)
{
    float flRemainder;

    if ((flRemainder = (g_flCollisionOne - flX * m_flInvXX * flX) - flY * m_flInvYY * flY) < g_flZeroCollisionObj)
    {
        return g_flCollisionOne;
    }

    return -(static_cast<float>(sqrt(static_cast<double>(flRemainder))) * m_flRadiusZ);
}

#define AXIS_BAND 0.0001

// Returns a pointer to m_vNormal, so it is not re-entrant; null only at the centre.
// FUNCTION: HYPERX 0x0042f120
Vec3* CollisionEllipsoid::NormalAtPoint(const Vec3* pPoint)
{
    Vec3* pNormalOut;
    float flAxis;

    if (pPoint->z <= AXIS_BAND && pPoint->z >= -AXIS_BAND)
    {
        if (pPoint->x <= AXIS_BAND && pPoint->x >= -AXIS_BAND)
        {
            if (pPoint->y <= AXIS_BAND && pPoint->y >= -AXIS_BAND)
            {
                return nullptr;
            }

            flAxis = m_flRadiusY;

            g_ellipsoidRatios.flDivisor = flAxis * flAxis / pPoint->y;
            g_ellipsoidRatios.flZ       = g_ellipsoidRatios.flDivisor * m_flInvZZ * pPoint->z;
            g_ellipsoidRatios.flX       = m_flInvXX * pPoint->x * g_ellipsoidRatios.flDivisor;

            if (pPoint->y > g_flZeroCollisionObj)
            {
                m_vNormal.y = -(g_flCollisionOne / static_cast<float>(sqrt(static_cast<double>((g_ellipsoidRatios.flZ * g_ellipsoidRatios.flZ) + g_ellipsoidRatios.flX * g_ellipsoidRatios.flX + g_flCollisionOne))));
            }
            else
            {
                m_vNormal.y = g_flCollisionOne / static_cast<float>(sqrt(static_cast<double>((g_ellipsoidRatios.flZ * g_ellipsoidRatios.flZ) + g_ellipsoidRatios.flX * g_ellipsoidRatios.flX + g_flCollisionOne)));
            }

            pNormalOut = &m_vNormal;

            pNormalOut->x = g_ellipsoidRatios.flX * m_vNormal.y;
            m_vNormal.z   = g_ellipsoidRatios.flZ * m_vNormal.y;

            return pNormalOut;
        }

        flAxis = m_flRadiusX;

        g_ellipsoidRatios.flDivisor = flAxis * flAxis / pPoint->x;
        g_ellipsoidRatios.flY       = m_flInvYY * pPoint->y * g_ellipsoidRatios.flDivisor;
        g_ellipsoidRatios.flZ       = g_ellipsoidRatios.flDivisor * m_flInvZZ * pPoint->z;

        if (pPoint->x > g_flZeroCollisionObj)
        {
            pNormalOut    = &m_vNormal;
            pNormalOut->x = -(g_flCollisionOne / static_cast<float>(sqrt(static_cast<double>((g_ellipsoidRatios.flZ * g_ellipsoidRatios.flZ) + g_ellipsoidRatios.flY * g_ellipsoidRatios.flY + g_flCollisionOne))));
        }
        else
        {
            pNormalOut    = &m_vNormal;
            pNormalOut->x = g_flCollisionOne / static_cast<float>(sqrt(static_cast<double>((g_ellipsoidRatios.flZ * g_ellipsoidRatios.flZ) + g_ellipsoidRatios.flY * g_ellipsoidRatios.flY + g_flCollisionOne)));
        }

        m_vNormal.z = g_ellipsoidRatios.flZ * pNormalOut->x;
        m_vNormal.y = g_ellipsoidRatios.flY * pNormalOut->x;

        return pNormalOut;
    }

    flAxis = m_flRadiusZ;

    g_ellipsoidRatios.flDivisor = flAxis * flAxis / pPoint->z;
    g_ellipsoidRatios.flY       = m_flInvYY * pPoint->y * g_ellipsoidRatios.flDivisor;
    g_ellipsoidRatios.flX       = m_flInvXX * pPoint->x * g_ellipsoidRatios.flDivisor;

    if (pPoint->z > g_flZeroCollisionObj)
    {
        m_vNormal.z = -(g_flCollisionOne / static_cast<float>(sqrt(static_cast<double>((g_ellipsoidRatios.flX * g_ellipsoidRatios.flX) + g_ellipsoidRatios.flY * g_ellipsoidRatios.flY + g_flCollisionOne))));
    }
    else
    {
        m_vNormal.z = g_flCollisionOne / static_cast<float>(sqrt(static_cast<double>((g_ellipsoidRatios.flX * g_ellipsoidRatios.flX) + g_ellipsoidRatios.flY * g_ellipsoidRatios.flY + g_flCollisionOne)));
    }

    pNormalOut = &m_vNormal;

    pNormalOut->x = g_ellipsoidRatios.flX * m_vNormal.z;
    m_vNormal.y   = g_ellipsoidRatios.flY * m_vNormal.z;

    return pNormalOut;
}

// The zero-direction floor.
#define DIRECTION_EPS 1e-08

// The third semi-axis is never read and /3 is a tuning constant; the plane's slot answers -1.
// FUNCTION: HYPERX 0x0042f350
float CollisionEllipsoid::RadiusForDirection(const Vec3* pVDir, int)
{
    const auto  flHorizontal = static_cast<float>((fabs(static_cast<double>(pVDir->x)) + fabs(static_cast<double>(pVDir->z))) / g_flTwoEllipsoid);
    const auto  flAbsY       = static_cast<float>(fabs(static_cast<double>(pVDir->y)));
    const float flTotal      = flAbsY + flHorizontal;

    if (flTotal < DIRECTION_EPS)
    {
        return g_flZeroCollisionObj;
    }

    return (flHorizontal * m_flRadiusX + flAbsY * m_flRadiusY) / (g_flThreeEllipsoid * flTotal);
}
