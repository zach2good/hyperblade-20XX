// Ages the owner's button timeouts, then passes the message down the chain.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "planvm/agendaitem.h"
#include "engine/cmonitorheldkeys.h"
#include "input/keyboarddeviceobj.h"

// FUNCTION: HYPERX 0x00410db0
void CMonitorHeldKeys::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind  = Agenda_CMonitorHeldKeys;
    m_pNext  = nullptr;
    m_bReady = 0;
    m_pOwner = pSubject;
}

// FUNCTION: HYPERX 0x00410dd0
void CMonitorHeldKeys::Process(int nArg)
{
    ((KeyboardDeviceObj*)m_pOwner)->CheckButtonTimeouts();

    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Process(nArg);
    }
}
