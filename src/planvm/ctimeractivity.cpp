// A timer whose chain points at itself is reported and terminated.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "engine/devicector.h"
#include "planvm/ctimeractivity.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "app/gameapp.h"
#include "app/bringup.h"
#include "plannode.h"
#include "common/pooledownerroot.h"
#include "engine/messageids.h"

// One past the last engine slot: no target.
#define TIMER_TARGET_NONE 0x66

// FUNCTION: HYPERX 0x00411a10
void CTimerActivity::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_pNext       = 0;
    m_wKind       = Agenda_CTimerActivity;
    m_bReady      = 0;
    m_pOwner      = pSubject;
    m_nFireTickMs = 0;
    m_nEvent      = MSG_NOMSG;
    m_nTargetSlot = TIMER_TARGET_NONE;
    m_nEventParam = 0;
}

// FUNCTION: HYPERX 0x00411a40
void CTimerActivity::BindWithNode(EngineObject* pObject, GameBehaviorObject*, PlanNode* pNode, int nArg)
{
    m_pNext  = 0;
    m_pOwner = pObject;
    m_wKind  = Agenda_CTimerActivity;
    m_bReady = 0;

    m_nFireTickMs = (int)(g_pGameEngine->m_clock.nTickMs + pNode->m_pArgs[0].n * 10);

    m_nEvent      = pNode->m_pArgs[1].n;
    m_nTargetSlot = pNode->m_pArgs[2].n;

    if (pNode->m_nArgCount == 4)
    {
        if (pNode->m_pArgs[3].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            m_nEventParam = nArg;
            return;
        }

        m_nEventParam = pNode->m_pArgs[3].n;
        return;
    }

    m_nEventParam = 0;
}

// FUNCTION: HYPERX 0x00411ac0
void CTimerActivity::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pOwner      = 0;
        m_nFireTickMs = 0;
        m_nEventParam = 0;
        m_pNext       = 0;
        m_nEvent      = MSG_NOMSG;
        m_nTargetSlot = TIMER_TARGET_NONE;
        m_bReady      = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x00411b00
void CTimerActivity::Process(int nArg)
{
#ifdef HYPERX_BRINGUP
    // tools/bringup_run.py --trace-obj follows one object's timers; the owner can be null here.
    if (m_pOwner != 0)
    {
        BringUp::TraceTimerStep(m_pOwner->m_pszName, m_bReady, m_nFireTickMs, m_nEvent);
    }
#endif

    if (m_bReady != 0)
    {
        Detach();

        return;
    }

    if (g_pGameEngine->m_clock.nTickMs > (unsigned int)m_nFireTickMs)
    {
        if (m_nTargetSlot == PLAN_TOKEN_BROADCAST)
        {
            g_pGameEngine->BroadcastEvent(m_nEvent, m_nEventParam, m_pOwner->GetSlot());
        }
        else
        {
            g_pGameEngine->m_ppSlots[(short)m_nTargetSlot]
                ->PostSceneEvent(m_nEvent, m_nEventParam);
        }

        if (this == m_pNext)
        {
            // STRING: HYPERX 0x004bb76c
            g_gameApp.Error("Timer Chasing Tail will be Terminated.");

            Detach();
        }

        if (m_pNext != 0 && m_bReady == 0)
        {
            m_pNext->Process(nArg);
        }

        Detach();
    }

    if (this == m_pNext)
    {
        g_gameApp.Error("Timer Chasing Tail will be Terminated.");

        Detach();
    }

    if (m_pNext != 0 && m_bReady == 0)
    {
        m_pNext->Process(nArg);
    }
}

// FUNCTION: HYPERX 0x00411c00
void CTimerActivity::Release(int nArg)
{
    if (g_pGameEngine->m_clock.nTickMs > (unsigned int)m_nFireTickMs)
    {
        if (m_nTargetSlot == PLAN_TOKEN_BROADCAST)
        {
            g_pGameEngine->BroadcastEvent(m_nEvent, m_nEventParam, m_pOwner->GetSlot());
        }
        else
        {
            g_pGameEngine->m_ppSlots[(short)m_nTargetSlot]
                ->PostSceneEvent(m_nEvent, m_nEventParam);
        }

        if (m_pNext != 0)
        {
            m_pNext->Process(nArg);
        }

        Detach();
    }

    if (m_pNext != 0 && m_bReady == 0)
    {
        m_pNext->Release(nArg);
    }
}
