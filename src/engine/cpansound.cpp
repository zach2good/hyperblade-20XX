// With m_bReady clear, a failing check is heard twice.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "engine/cpansound.h"
#include <cstdlib>
#include "globals.h"
#include "common/pooledownerroot.h"
#include "sound/soundmanager.h"

// FUNCTION: HYPERX 0x00412260
void CPanSound::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_pNext      = nullptr;
    m_wKind      = Agenda_CPanSound;
    m_bReady     = 0;
    m_pOwner     = pSubject;
    m_pSound     = nullptr;
    m_nPan       = 0;
    m_nPanTarget = 0;
    m_nPanStep   = 0;
}

// FUNCTION: HYPERX 0x00412290
void CPanSound::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pOwner     = nullptr;
        m_pSound     = nullptr;
        m_nPan       = 0;
        m_nPanTarget = 0;
        m_nPanStep   = 0;
        m_pNext      = nullptr;
        m_bReady     = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x004122d0
void CPanSound::Process(int nArg)
{
    if (m_pSound == nullptr || m_pSound->m_bPannable == 0 || abs(m_nPan - m_nPanTarget) <= abs(m_nPanStep))
    {
        if (m_pNext != nullptr)
        {
            m_pNext->Process(nArg);
        }

        Detach();
    }

    if (m_pNext != nullptr && m_bReady == 0)
    {
        m_pNext->Process(nArg);
    }
}
