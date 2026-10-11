#include "common/compat.h"
#include "engine/messageids.h"
#include "objectclasses.h"
#include "objectdef.h"
#include "devicector.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "math/transformobject.h"
#include "gameengine.h"
#include "objectstats.h"
#include "statids.h"
#include "physics/physicsbody.h"
#include "math/vector.h"
#include "graphics/meshbone.h"
#include <math.h>
#include <stdlib.h>
#include "playerobject.h"
#include "common/pooledclasses.h"
#include "engine/engineobject.h"
#include "engine/objectphase.h"

// Two rotations stage their cosine and sine in m_transformB; a skater copies 63 ratings, a goalie 16.
// The home team's players default to the keyboard's model id and the away team's to the gamepad's.
// FUNCTION: HYPERX 0x00474150
PlayerObject::PlayerObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    ApplyRenderSlotPose();

    // The client holds the transform in EDI (0x004741c5).
    TransformObject* const pLocal = &m_transformB;

    const float flAngleX = g_flDegToRadPlayer * pDef->m_flElevation;

    pLocal->m_rotX.flCos = (float)cos(flAngleX);
    pLocal->m_rotX.flSin = (float)sin(flAngleX);

    pLocal->RotateXStaged();

    const float flAngleZ = g_flDegToRadPlayer * pDef->m_flAzimuth;

    pLocal->m_rotZ.flCos = (float)cos(flAngleZ);
    pLocal->m_rotZ.flSin = (float)sin(flAngleZ);

    pLocal->ApplyRotationZ();

    const int nLevel = g_pGameEngine->GetDifficultyLevel();
    int       i;

    if (pDef->m_nRatingsMode == RATINGS_FIELD)
    {
        const float* pSource = pDef->m_apRatings[nLevel];
        for (i = 0; i < RATING_COUNT; i++)
        {
            m_stats.SetStat(i + 1, *pSource);
            pSource++;
        }

        m_stats.ScaleTo(nSlot);
    }
    else if (pDef->m_nRatingsMode == RATINGS_GOALIE)
    {
        const float* pSpecial;
        const float* pSource;

        if ((nSlot != GOALIE_SLOT_A) || (nLevel != 0) || ((pSpecial = pDef->m_apRatings[RATINGS_SPECIAL_GOALIE]) == 0) || (g_bSessionActive != 0))
        {
            pSource = pDef->m_apRatings[nLevel];
        }
        else
        {
            pSource = pSpecial;
        }

        for (i = 0; i < GOALIE_RATING_COUNT; i++)
        {
            m_stats.SetStat(i + 1, *pSource);
            pSource++;
        }
    }

    m_wAttackeeSlot     = -1;
    m_nKnockedDownState = 0;

    if ((m_nSlot == SLOT_DROMER0) || (m_nSlot == SLOT_DROMER2) || (m_nSlot == GOALIE_SLOT_A))
    {
        m_nTypeId = SLOT_KEYBOARD;
    }
    else
    {
        m_nTypeId = SLOT_GAMEPAD0;
    }

    m_wKilledBySlot          = -1;
    m_wAnimationSemaphore    = 0;
    m_wClockStoppedSemaphore = 0;
    m_wDesiredSubstitute     = -1;
    m_wFlyingSemaphore       = 0;
}

#define DAMAGE_UPDATE_INTERVAL 2000

// FUNCTION: HYPERX 0x004742f0
inline void PlayerObject::ApplyRenderSlotPose()
{
    UpdateRenderSlotTransform(m_wRenderSlot);
}

// FUNCTION: HYPERX 0x00474320
PlayerObject::~PlayerObject()
{
}

// At most every two seconds; the damage is truncated to a byte, so 256 or more wraps.
// FUNCTION: HYPERX 0x00474330
void PlayerObject::Postpass()
{
    if (g_bSessionActive != 0 && g_pGameEngine != 0)
    {
        if (GetPhase() == OBJECT_PHASE_LOCAL)
        {
            if (GetSlot() <= SLOT_DROMER3 && (unsigned int)(m_nLastSendMs + DAMAGE_UPDATE_INTERVAL) < g_pGameEngine->m_clock.nTickMs)
            {
                int nDamage = (int)m_stats.GetScalar(RATING_DAMAGE);
                nDamage &= 0xff;

                PostSceneMessage(MSG_SYNC_DAMAGE, nDamage, GetSlot(), 0);

                m_nLastSendMs = g_pGameEngine->m_clock.nTickMs;
            }
        }
    }
}

// One static scratch transform serves every object; 0x00471630 is the same source in another unit.
// FUNCTION: HYPERX 0x004743d0
void PlayerObject::UpdateRenderSlotTransform(short nSlotIdx)
{
    // The original's object at 0x004d0658 takes no marker; symbols/static_guards.csv names its guard.
    static TransformObject BoneTransform;

    m_nFlags |= ENGINEOBJECT_POSED;
    MeshBone* const pSlot = &m_pBones[nSlotIdx];

    m_transformB.Multiply(&m_transformC, &m_transformA);

    m_transformC.AddTranslationVec(&m_pBody->pos);

    pSlot->PropagateTransforms(nSlotIdx, &m_transformC, &m_transformC, &BoneTransform);
}
