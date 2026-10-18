#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "planvm/agendaitem.h"
#include "planvm/cdopplersnd.h"
#include "globals.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x00412700
void CDopplerSnd::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_pNext    = nullptr;
    m_wKind    = Agenda_CDopplerSnd;
    m_bReady   = 0;
    m_pOwner   = pSubject;
    m_nUnusedA = 0;
    m_nUnusedB = 0;
    m_nUnusedC = 0;
}

// FUNCTION: HYPERX 0x00412730
void CDopplerSnd::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_nUnusedA = 0;
        m_pOwner   = nullptr;
        m_nUnusedB = 0;
        m_nUnusedC = 0;
        m_pNext    = nullptr;
        m_bReady   = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x00412760
void CDopplerSnd::Process(int nArg)
{
    AgendaItem* const pChain = m_pNext;

    if (pChain != nullptr && m_bReady == 0)
    {
        pChain->Process(nArg);
    }
}
