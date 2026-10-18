// The second coach does nothing, the first runs the coach AI, and anything else runs the chain.

#include "common/compat.h"
#include "engine/objectclasses.h"
#include "planvm/agendaitemids.h"
#include "gameengine.h"
#include "planvm/agendaitem.h"
#include "engine/ccheckai.h"
#include "devicector.h"
#include "globals.h"
#include "engine/engineobject.h"

// FUNCTION: HYPERX 0x00412b10
void CCheckAI::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind  = Agenda_CCheckAI;
    m_pNext  = nullptr;
    m_bReady = 0;
    m_pOwner = pSubject;
}

// FUNCTION: HYPERX 0x00412b30
void CCheckAI::Process(int nArg)
{
    EngineObject** const ppObjects = g_pGameEngine->m_ppSlots;

    EngineObject* const pOwner = m_pOwner;

    EngineObject* const pCoach = ppObjects[SLOT_COACH0];

    if (pOwner == ppObjects[SLOT_COACH1])
    {
        return;
    }

    if (pOwner == pCoach)
    {
        pCoach->UpdateCoachAI();

        return;
    }

    pCoach->CheckObjectAI(pOwner);

    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr && m_bReady == 0)
    {
        pNext->Process(nArg);
    }
}
