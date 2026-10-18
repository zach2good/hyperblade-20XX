#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "planvm/agendaitem.h"
#include "planvm/cviewit.h"
#include "engine/cameraobject.h"

// FUNCTION: HYPERX 0x0040b690
void CViewIt::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind  = Agenda_CViewIt;
    m_pNext  = nullptr;
    m_bReady = 0;
    m_pOwner = pSubject;
}

// FUNCTION: HYPERX 0x0040b6b0
void CViewIt::Process(int nArg)
{
    GetCamera()->ViewIt();

    auto* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Process(nArg);
    }
}
