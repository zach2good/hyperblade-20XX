#ifndef HYPERBLADE_CTIMERACTIVITY_H
#define HYPERBLADE_CTIMERACTIVITY_H

#include "planvm/kindagendaitem.h"

class EngineObject;
struct PlanNode;

// Slots 7 and 8 must answer one here, or the Rok would sit in the start gun for the whole match.
// VTABLE: HYPERX 0x004b1f0c
class CTimerActivity : public KindAgendaItem
{
public:
    void Process(int nArg) override;
    void Release(int nArg) override;
    void BindWithNode(class EngineObject* pObject, class GameBehaviorObject* pOwner, PlanNode* pNode, int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    unsigned char GetReadyFlag() override
    {
        return 1;
    }

    // Slot eight answering one keeps several timers of one kind; nought would evict all but the last.
    char IsReplaceable() override
    {
        return 1;
    }

    // No macro names this run; the reads agree on the machine word at every offset.
    int m_nFireTickMs; // 0x10
    // The event id and the word that rides with it; the default binds seed MSG_NOMSG.
    int m_nEvent; // 0x14
    // 0xffff broadcasts to every object of the owner's kind; anything else indexes the slot array.
    int m_nTargetSlot; // 0x18
    int m_nEventParam; // 0x1c
};

#endif
