#include "common/compat.h"
#include "sound/soundmanager.h"
#include "globals.h"
#include "planvm/plannode.h"
#include "engine/gameengine.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "planvm/planvm.h"
#include "math/vector.h"
#include "planvm/damagegate.h"
#include "sound/audiomgr.h"
#include <cmath>
#include "engine/engineobject.h"
#include "anim/animevent.h"
#include "sound/soundids.h"

// An index past the bank's count writes through a null pointer and faults.

// FUNCTION: HYPERX 0x00406ea0
void AnimEvent::PlaySnd(Interactor*, PlanNode* pEvent, short*, int)
{
    SoundEntry* pSlot;

    const auto nIndex = pEvent->m_pArgs[1].n;

    auto* const pBank = g_pAudioMgr->pEntries.get();

    if (nIndex < pBank->m_nSoundCount)
    {
        pSlot = pBank->m_apSounds[nIndex].get();
    }
    else
    {
        pSlot = nullptr;
    }

    pSlot->m_nVolume = 0;
    pSlot->m_nPan    = 0;

    g_pAudioMgr->PlaySpecific(pSlot->m_nIndex);
}

// A tag of _MESSAGE_DATA_ sends the caller's own argument as the group.
// FUNCTION: HYPERX 0x00406ef0
void AnimEvent::PlaySndType(Interactor*, PlanNode* pArgs, short*, int nGroup)
{
    const auto nTag = pArgs->m_pArgs[1].n;

    if (nTag == PLAN_TOKEN_MESSAGE_DATA)
    {
        g_pAudioMgr->PlayGroup(nGroup);
    }
    else
    {
        g_pAudioMgr->PlayGroup(nTag);
    }
}

// id_ACF_PanSndType in DATA/ENUMIDS.H: the plan action table registers this body at 13.
// FUNCTION: HYPERX 0x00406f20
void AnimEvent::PanSndType(Interactor*, PlanNode* pEvent, short*, int nGroup)
{
    if (g_pAudioMgr->wNoSound != 0)
    {
        return;
    }

    const auto nEventGroup = pEvent->m_pArgs[1].n;

    if (nEventGroup == PLAN_TOKEN_MESSAGE_DATA)
    {
        g_pAudioMgr->PlayGroup(nGroup);
        return;
    }

    g_pAudioMgr->PlayGroup(nEventGroup);
}

// id_ACF_DopplerSound, plan action 51: plays the sound with no distance test.
// FUNCTION: HYPERX 0x00406f50
void AnimEvent::DopplerSound(Interactor*, PlanNode* pArgs, short*, int)
{
    if (g_pAudioMgr->wNoSound == 0)
    {
        g_pAudioMgr->PlaySpecific(pArgs->m_pArgs[1].n);
    }
}

// Plays while the emitter is within about 267 units, quieter the further it is.
// FUNCTION: HYPERX 0x00406f70
void AnimEvent::PlaySoundByDistance(Interactor* pContext, PlanNode* pEvent, short*, int)
{
    SoundEntry* pSlot;
    Vec3        vDelta;
    float       flScaled;

    const auto nIndex = pEvent->m_pArgs[1].n;

    auto* const pBank = g_pAudioMgr->pEntries.get();

    if (nIndex < pBank->m_nSoundCount)
    {
        pSlot = pBank->m_apSounds[nIndex].get();
    }
    else
    {
        pSlot = nullptr;
    }

    auto* const pListener = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];

    const Vec3* const pListenerPos = pListener->GetPositionPtr();
    const Vec3* const pEmitterPos  = pContext->GetActor()->GetPositionPtr();

    vDelta.x = pEmitterPos->x - pListenerPos->x;
    vDelta.y = pEmitterPos->y - pListenerPos->y;
    vDelta.z = pEmitterPos->z - pListenerPos->z;

    flScaled = static_cast<float>(sqrt(static_cast<double>(Vec3::DotInline(&vDelta, &vDelta)))) * -30.0f;

    if (flScaled > -8000.0f)
    {
        pSlot->m_nVolume = static_cast<int>(flScaled);
        Math::Vec3Normalize(&vDelta);
        pListener->GetPhysicsBody();
        pSlot->m_nPan = 0;
        g_pAudioMgr->PlaySpecific(pSlot->m_nIndex);
    }
}

// id_DCF_IsAmbient in DATA/ENUMIDS.H: the plan opcode table registers this body at 60.
// FUNCTION: HYPERX 0x00407050
void AnimEvent::IsAmbient(Interactor*, PlanNode*, short* pResult, int nEntry)
{
    SoundEntry* pEntry;

    if (nEntry < g_pAudioMgr->pEntries->m_nSoundCount)
    {
        pEntry = g_pAudioMgr->pEntries->m_apSounds[nEntry].get();
    }
    else
    {
        pEntry = nullptr;
    }

    // Unguarded: a failed lookup is read through, as in the original.
    *pResult = pEntry->m_nGroup != kSoundGroupLooping;

    if (g_pAudioMgr->IsPlaying(kSoundSxxambb2) == 0)
    {
        *pResult = 2;
    }
}
