#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "planvm/agendaitem.h"
#include "planvm/cstepanimation.h"
#include "engine/engineobject.h"
#include "anim/animplayer.h"

// FUNCTION: HYPERX 0x00410d30
void CStepAnimation::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind  = Agenda_CStepAnimation;
    m_pNext  = 0;
    m_bReady = 0;
    m_pOwner = pSubject;
}

// FUNCTION: HYPERX 0x00410d50
void CStepAnimation::Process(int nArg)
{
    m_pOwner->m_pAnimPlayer->UpdateTick();

    AgendaItem* const pNext = m_pNext;

    if (pNext != 0)
    {
        pNext->Process(nArg);
    }
}

// FUNCTION: HYPERX 0x00410d80
void CStepAnimation::Release(int nArg)
{
    m_pOwner->m_pAnimPlayer->UpdateTick();

    AgendaItem* const pNext = m_pNext;

    if (pNext != 0)
    {
        pNext->Release(nArg);
    }
}
