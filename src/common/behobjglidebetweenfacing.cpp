#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "engine/engineobject.h"
#ifdef HYPERX_V100
#include "engine/gameengine.h"
#endif

// FUNCTION: HYPERX100 0x004ca8c0
// FUNCTION: HYPERX 0x0041d6a0
BehObj_GlideBetweenFacing::BehObj_GlideBetweenFacing()
{
}

// FUNCTION: HYPERX100 0x004ca980
// FUNCTION: HYPERX 0x0041d6e0
BehObj_GlideBetweenFacing::~BehObj_GlideBetweenFacing()
{
}

// FUNCTION: HYPERX100 0x004ca9f0
// FUNCTION: HYPERX 0x0041d6f0
void BehObj_GlideBetweenFacing::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForGlideBetweenFacing();

    // STRING: HYPERX 0x004bc2a8
    m_pszName = "GlideBetweenFacing";

    m_wKind = BEH_GLIDE_BETWEEN_FACING;

    m_apSlots[0] = AllocAgendaItem(Agenda_CAdjustAcceleration);

    m_apSlots[1] = AllocAgendaItem(Agenda_CSteerBetweenFacing);
    m_apSlots[2] = AllocAgendaItem(Agenda_CGlideOnSurface);
    m_apSlots[3] = AllocAgendaItem(Agenda_CStepAnimation);
    m_apSlots[4] = AllocAgendaItem(Agenda_CSetAttitude);

    short nSlot  = 0;
    m_apSlots[5] = 0;

    m_pOwner = pOwner;
    m_pBody  = pOwner->GetPhysicsBody();

    while (m_apSlots[nSlot] != 0)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        AgendaItem* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);
}

#ifdef HYPERX_V100
// FUNCTION: HYPERX100 0x004caaf0
void BehObj_GlideBetweenFacing::SetObjects(short nSlotA, short nSlotB, short nAimSlot, float flBlendStep)
{
    m_pObjectA    = g_pGameEngine->m_ppSlots[nSlotA];
    m_pObjectB    = g_pGameEngine->m_ppSlots[nSlotB];
    m_pAimObject  = g_pGameEngine->m_ppSlots[nAimSlot];
    m_flBlendStep = flBlendStep;
}
#endif
