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
        g_gameApp.FatalString((const char*)static_cast<std::uintptr_t>(static_cast<unsigned short>(STRING_ZERO_NORMAL_IN_PLANE)));
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
        g_gameApp.FatalString((const char*)static_cast<std::uintptr_t>(static_cast<unsigned short>(STRING_ZERO_NORMAL_IN_PLANE)));
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
            // STRING: HYPERX 0x004c45cc
            g_gameApp.Fatal("ColObj to Model map unaccounted for.\n");

            return MODEL_ID_NONE;
    }
}

// FUNCTION: HYPERX 0x00480740
CollisionObj::CollisionObj(const Vec3* pOrigin, const Vec3* pB, const Vec3* pC, float flFriction, float flElasticity)
{
    InitFrom3Points(pOrigin, pB, pC, flFriction, flElasticity);
}
