#ifndef HYPERBLADE_INTERACTOR_H
#define HYPERBLADE_INTERACTOR_H

#include "common/compat.h"
#include "planvm/plannode.h"

class EngineObject;
class PlayerObject;
class CameraObject;
struct PlanNode;

// The base of the collision and message interactors; the re-arm walk calls only slot 10.
// VTABLE: HYPERX 0x004b37e8
class Interactor
{
public:
    // The eight words both pools write before the kind's own three.
    Interactor()
    : m_pOwner(nullptr)
    , m_nUnused(0)
    , m_pNode(nullptr)
    , pNext(nullptr)
    , m_nActiveState(0)
    , m_nBehaviour(0)
    , m_nMember(0)
    , m_bEnabled(1)
    {
    }

    Interactor(int nOtherSlot, int nOwnSlotOrMessage, int nSeed, int bMessage)
    : m_pOwner(nullptr)
    , m_nUnused(0)
    , m_pNode(nullptr)
    , pNext(nullptr)
    , m_nActiveState(0)
    , m_nBehaviour(0)
    , m_nMember(0)
    , m_bEnabled(1)
    {
        m_otherSlot.n        = nOtherSlot;
        m_ownSlotOrMessage.n = nOwnSlotOrMessage;

        if (bMessage != 0)
        {
            m_wHandlerIndex = static_cast<short>(nSeed);
        }
        else
        {
            m_wEventKind = static_cast<short>(nSeed);
        }
    }

    virtual ~Interactor() = default;

    // Object::Trace, which eleven tables share; CollInteractor overrides it.
    virtual void Discard()
    {
    }

    virtual int GetActiveState();

    // FUNCTION: HYPERX 0x0046bb50
    virtual void SetMember(int nMember)
    {
        m_nMember = nMember;
    }

    // Zero, one or two: the phase the object below is checked against.
    virtual int           GetState();
    virtual EngineObject* GetActor();
    // The actor as the class the caller knows it to be; each is defined beside its class.
    PlayerObject* GetPlayerActor();
    CameraObject* GetCameraActor();

    // The message id EngineObject::RegisterSceneMessageHandler indexes its byte table by.
    // FUNCTION: HYPERX 0x0046bba0
    virtual short GetNamedSlot()
    {
        return -1;
    }

    virtual short GetOtherSlot()
    {
        return -1;
    }

    virtual short GetModelSlot()
    {
        return -1;
    }

    virtual short GetTargetSlot()
    {
        return -1;
    }

    virtual void SetActiveState(int nState);

    void ExecuteScript(union PlanWord* pCursor, short nCount, int nArg, short* pState);

    // Most readers use 0x004 as a pointer; five read it as a short.
    class EngineObject* m_pOwner; // 0x004
    // Only the constructors touch it.
    int       m_nUnused; // 0x008
    PlanNode* m_pNode;   // 0x00c
    // The base's type: both interactors chain through this word.
    Interactor* pNext; // 0x010
    // id_SET_ActiveState's value; minus one disables the interactor and every walk skips it.
    int   m_nActiveState; // 0x014
    int   m_nBehaviour;   // 0x018
    int   m_nMember;      // 0x01c
    short m_wSlot;        // 0x020
    short m_wBehaviour;   // 0x022
    char  m_bEnabled;     // 0x024
    // Each a plan word, which a collision interactor resolves when it is a pronoun.
    // A collision interactor's other and own col-object slots; a message interactor's sender and message.
    PlanWord m_otherSlot;        // 0x028
    PlanWord m_ownSlotOrMessage; // 0x02c
    // MsgInteractor's pool fill seeds this and CollInteractor's does not; nothing else reads it.
    short m_wHandlerIndex; // 0x030
    short m_wEventKind;    // 0x032
};

// SYNTHETIC: HYPERX 0x0046bbb0
// Interactor::`scalar deleting destructor'

#endif
