#ifndef HYPERBLADE_PLAYEROBJECT_H
#define HYPERBLADE_PLAYEROBJECT_H

#include "common/compat.h"
#include "objectclasses.h"
#include "engine/devicector.h"
#include "engine/objectstats.h"
#include "engine/engineobject.h"
#include "engine/matchstats.h"
#include "engine/planfixes.h"
#include "engine/gameengine.h"
#include "planvm/interactor.h"

// VTABLE: HYPERX 0x004b3af4
class PlayerObject : public EngineObject
{
public:
    PlayerObject(int nSlot, class ObjectDef* pDef);

    ~PlayerObject() override;

    // The three slots 0x004b3af4 overrides from 0x004b38f0. The bot shares two of them.
    void ApplyRenderSlotPose() override;
    void Postpass() override;
    void UpdateRenderSlotTransform(short nSlotIdx) override;

    // The goal this player shoots at, kSlotNgoal or kSlotSgoal; three below it is that goal's keeper.
    short m_wTargetGoalSlot; // 0xd14
    short m_wUnused;         // 0xd16
    // Written by id_ACF_SetAttackee and id_ACF_SetKnockedDownState; minus one and nought at first.
    short       m_wAttackeeSlot;     // 0xd18
    int         m_nKnockedDownState; // 0xd1c
    ObjectStats m_stats;
    MatchStats  m_matchStats;

    // The slot of whoever killed this dromer, minus one for nobody.
    short m_wKilledBySlot; // 0xfe8

    // Four semaphores named by DATA/ENUMIDS.H.
    short m_wAnimationSemaphore;    // 0xfea
    short m_wClockStoppedSemaphore; // 0xfec
    // The model to substitute to, or minus one.
    short m_wDesiredSubstitute; // 0xfee
    short m_wFlyingSemaphore;   // 0xff0

    PowerUpRestore m_powerUpRestore;
};

// SYNTHETIC: HYPERX 0x00474300
// PlayerObject::`scalar deleting destructor'

inline PlayerObject* GameEngine::GetPlayer(int nSlot) const
{
    return (PlayerObject*)m_ppSlots[nSlot];
}

inline PlayerObject* Interactor::GetPlayerActor()
{
    return (PlayerObject*)GetActor();
}

// The atexit thunk for UpdateRenderSlotTransform's scratch transform.
// SYNTHETIC: HYPERX 0x00474480
// $E2

#endif
