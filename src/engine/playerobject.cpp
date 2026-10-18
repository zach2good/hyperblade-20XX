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
#include <cmath>
#include <cstdlib>
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
    auto* const pLocal = &m_transformB;

    const auto flAngleX = g_flDegToRadPlayer * pDef->m_flElevation;

    pLocal->m_rotX.flCos = static_cast<float>(cos(static_cast<double>(flAngleX)));
    pLocal->m_rotX.flSin = static_cast<float>(sin(static_cast<double>(flAngleX)));

    pLocal->RotateXStaged();

    const auto flAngleZ = g_flDegToRadPlayer * pDef->m_flAzimuth;

    pLocal->m_rotZ.flCos = static_cast<float>(cos(static_cast<double>(flAngleZ)));
    pLocal->m_rotZ.flSin = static_cast<float>(sin(static_cast<double>(flAngleZ)));

    pLocal->ApplyRotationZ();

    const auto nLevel = g_pGameEngine->GetDifficultyLevel();
    int        i;

    if (pDef->m_nRatingsMode == kRatingsField)
    {
        const float* pSource = pDef->m_apRatings[nLevel];
        for (i = 0; i < kRatingCount; i++)
        {
            m_stats.SetStat(i + 1, *pSource);
            pSource++;
        }

        m_stats.ScaleTo(nSlot);
    }
    else if (pDef->m_nRatingsMode == kRatingsGoalie)
    {
        const float* pSpecial;
        const float* pSource;

        if ((nSlot != kGoalieSlotA) || (nLevel != 0) || ((pSpecial = pDef->m_apRatings[kRatingsSpecialGoalie]) == nullptr) || (g_bSessionActive != 0))
        {
            pSource = pDef->m_apRatings[nLevel];
        }
        else
        {
            pSource = pSpecial;
        }

        for (i = 0; i < kGoalieRatingCount; i++)
        {
            m_stats.SetStat(i + 1, *pSource);
            pSource++;
        }
    }

    m_wAttackeeSlot     = -1;
    m_nKnockedDownState = 0;

    if ((m_nSlot == kSlotDromer0) || (m_nSlot == kSlotDromer2) || (m_nSlot == kGoalieSlotA))
    {
        m_nTypeId = kSlotKeyboard;
    }
    else
    {
        m_nTypeId = kSlotGamepad0;
    }

    m_wKilledBySlot          = -1;
    m_wAnimationSemaphore    = 0;
    m_wClockStoppedSemaphore = 0;
    m_wDesiredSubstitute     = -1;
    m_wFlyingSemaphore       = 0;
}

constexpr auto kDamageUpdateInterval = 2000;

// FUNCTION: HYPERX 0x004742f0
inline void PlayerObject::ApplyRenderSlotPose()
{
    UpdateRenderSlotTransform(m_wRenderSlot);
}

// FUNCTION: HYPERX 0x00474320
PlayerObject::~PlayerObject() = default;

// At most every two seconds; the damage is truncated to a byte, so 256 or more wraps.
// FUNCTION: HYPERX 0x00474330
void PlayerObject::Postpass()
{
    if (g_bSessionActive != 0 && g_pGameEngine != nullptr)
    {
        if (GetPhase() == OBJECT_PHASE_LOCAL)
        {
            if (GetSlot() <= kSlotDromer3 && static_cast<unsigned int>(m_nLastSendMs + kDamageUpdateInterval) < g_pGameEngine->m_clock.nTickMs)
            {
                auto nDamage = static_cast<int>(m_stats.GetScalar(kRatingDamage));
                nDamage &= 0xff;

                PostSceneMessage(kMsgSyncDamage, nDamage, GetSlot(), 0);

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

    m_nFlags |= kEngineObjectPosed;
    auto* const pSlot = &m_pBones[nSlotIdx];

    m_transformB.Multiply(&m_transformC, &m_transformA);

    m_transformC.AddTranslationVec(&m_pBody->pos);

    pSlot->PropagateTransforms(nSlotIdx, &m_transformC, &m_transformC, &BoneTransform);
}
