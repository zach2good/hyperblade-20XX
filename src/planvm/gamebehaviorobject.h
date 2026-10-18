#ifndef HYPERBLADE_GAMEBEHAVIOROBJECT_H
#define HYPERBLADE_GAMEBEHAVIOROBJECT_H

#include "planvm/agendaitemids.h"

class EngineObject;

// The ten agenda item slots a behaviour carries.
#define BEHAVIOR_SLOT_COUNT 10

// m_wLatency until id_SET_InteractorLatency sets it.
#define BEHAVIOR_DEFAULT_LATENCY 0x0a

// VTABLE: HYPERX 0x004b23c8
class GameBehaviorObject
{
public:
    // The table has _purecall at slot three, so the class is abstract.
    GameBehaviorObject();

    void FillAgendaPurged()
    {
        int nAt;

        for (nAt = 0; nAt < Agenda_Count; nAt++)
        {
            m_abAgendaPurged[nAt] = 1;
        }
    }

    void SetAgendaPurged(int nAt, char cValue)
    {
        m_abAgendaPurged[nAt] = cValue;
    }

    void SetAnimationPlayback(float flRate, int nFlags)
    {
        m_flAnimationRate = flRate;
        m_nAnimationFlags = nFlags;
    }

    void SetPersistentPlayback(float flRate, int nFlags)
    {
        m_flPersistentRate = flRate;
        m_nPersistentFlags = nFlags;
    }

    const char* GetName() const
    {
        return m_pszName;
    }

    int CanPassOn() const
    {
        return m_pCollInteractors == nullptr && m_pMsgInteractors == nullptr && m_pStandby != nullptr;
    }

    // Each takes out the agenda items the behaviour it is named for does not run.
    void PurgeAgendaForGlideToObject();
    void PurgeAgendaForGlideToPosition();
    void PurgeAgendaForGlideBetweenFacing();
    void PurgeAgendaForGlide();
    void PurgeAgendaForRailSlide();
    void PurgeAgendaForFly();
    void PurgeAgendaForUpdateWhileAttached();
    void PurgeAgendaForSetMovingInPlace();
    void PurgeAgendaForTracking();
    void PurgeAgendaForWatching();
    void PurgeAgendaForTrackTrajectory();
    void PurgeAgendaForTrackPassPrep();
    void PurgeAgendaForCandyTracking();

    virtual ~GameBehaviorObject();

    virtual void Init(class EngineObject* pOwner, struct PlanNode* pBlock, int nFlags);
    void         AssignPriorities(int nStart);

    void UpdateLimpAnim();

    virtual void Reset();
    // The motion a behaviour starts in; src/engine/rokobject.cpp calls it when behaviour nought activates.
    virtual void StartMotion() = 0;
    virtual void SendTimedMessage();
    virtual void SetInteractorsActive(int nArg);

    void TickAndSortMessages();

    // REMOVE_AGENDA_ITEM: takes every agenda item of that id off the owner's observer list.
    void RemoveAgendaItem(int nAgendaItem);

    void UpdateBehaviorPollCountdown();

    short m_wKind;      // 0x04
    short m_wSyncValue; // 0x06

    // The ten agenda item slots each BehObj_ initialiser binds.
    class AgendaItem*   m_apSlots[BEHAVIOR_SLOT_COUNT]; // 0x08
    struct PhysicsBody* m_pBody;                        // 0x30
    class EngineObject* m_pOwner;                       // 0x34
    // The animation the behaviour plays, and the persistent one RestorePersistentAnimation returns
    // to, each with its rate and flags.
    class AnimDB* m_pAnimation;           // 0x38
    class AnimDB* m_pPersistentAnimation; // 0x3c
    float         m_flAnimationRate;      // 0x40
    float         m_flPersistentRate;     // 0x44
    int           m_nAnimationFlags;      // 0x48
    int           m_nPersistentFlags;     // 0x4c
    // BindAnimEventChannel's channel, target, kind and parameter; the channel is never read.
    int m_nChannelSlot; // 0x50
    int m_nEventKind;   // 0x54
    int m_nEventTarget; // 0x58
    int m_nEventParam;  // 0x5c
    // The name the plan machinery looks a behaviour up by.
    const char*           m_pszName;          // 0x60
    class CollInteractor* m_pCollInteractors; // 0x64
    class MsgInteractor*  m_pMsgInteractors;  // 0x68
    // The next behaviour in the chain a plan state activates through.
    GameBehaviorObject* m_pStandby;       // 0x6c
    short               m_wPollCountdown; // 0x70
    short               m_wLatency;       // 0x72
    char                m_bDigressive;    // 0x74

protected:
    // A purge list, one byte per agenda id: a raised byte takes that item out.
    char m_abAgendaPurged[Agenda_Count]; // 0x75
};

// SYNTHETIC: HYPERX 0x0041bfc0
// GameBehaviorObject::`scalar deleting destructor'

#endif
