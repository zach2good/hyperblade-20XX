#ifndef HYPERBLADE_MSGINTERACTOR_H
#define HYPERBLADE_MSGINTERACTOR_H

#include "common/compat.h"
#include "planvm/interactor.h"
#include "engine/messageids.h"

// VTABLE: HYPERX 0x004b3814
class MsgInteractor : public Interactor
{
public:
    MsgInteractor()
    : Interactor(0, MSG_NOMSG, -1, 1)
    {
    }

    void Discard() override;

    // Slot six: the message tag Setup stores in m_ownSlotOrMessage.
    short GetNamedSlot() override
    {
        return static_cast<short>(m_ownSlotOrMessage.n);
    }

    // Where the base answers minus one, this reads m_otherSlot.
    short GetTargetSlot() override;

    MsgInteractor* Setup(class EngineObject* pObject, struct PlanNode* pInstr);
    void           SetSenderAndDispatch(int nArg, short* pState, int nSender);
    void           Dispatch(int nArg, short* pState);
};

#endif
