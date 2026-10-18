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

float g_flUnusedPlanA = g_flZeroPlan;
float g_flUnusedPlanB = g_flZeroPlan;
float g_flUnusedPlanC = g_flZeroPlan;

// FUNCTION: HYPERX 0x0042e640
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

    m_nShapeKind   = SHAPE_KIND_PLANE;
    m_flFriction   = flFriction;
    m_flElasticity = flElasticity;
    m_bCached      = 0;
}

// The failure path passes a string resource id where a format string belongs.
// FUNCTION: HYPERX 0x0042e710
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
        g_gameApp.FatalString((const char*)static_cast<std::uintptr_t>(static_cast<unsigned short>(kStringZeroNormalInPlane)));
    }

    m_vPoint = *pOrigin;

    m_nShapeKind   = SHAPE_KIND_PLANE;
    m_flFriction   = flFriction;
    m_flElasticity = flElasticity;
}

// Maps a CollisionObjectID to its ModelType; an unknown id is fatal.
// FUNCTION: HYPERX 0x0045e200
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
            // STRING: HYPERX 0x004c45cc
            g_gameApp.Fatal("ColObj to Model map unaccounted for.\n");

            return kModelIDNone;
    }
}

// FUNCTION: HYPERX 0x00480740
CollisionObj::CollisionObj(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity)
{
    InitFrom3Points(pOrigin, pB, pC, flFriction, flElasticity);
}
