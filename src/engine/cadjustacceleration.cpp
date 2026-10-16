#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "engine/cadjustacceleration.h"
#include "engineobject.h"
#include "globals.h"
#include "physics/physicsbody.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x0040f740
void CAdjustAcceleration::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = 0;
    m_wKind  = Agenda_CAdjustAcceleration;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pVelDir               = &pOwner->GetPhysicsBody()->velDir;
    m_pSmoothVel            = &pOwner->GetPhysicsBody()->smoothVel;
    float* const pflAccelDt = &pOwner->GetPhysicsBody()->flAccelDt;

    m_pTargetPos = 0;
    m_pflStep    = pflAccelDt;
}

// FUNCTION: HYPERX 0x0040f7a0
void CAdjustAcceleration::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext      = 0;
        m_pOwner     = 0;
        m_pVelDir    = 0;
        m_pSmoothVel = 0;
        m_pflStep    = 0;
        m_pTargetPos = 0;
        m_bReady     = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x0040f7e0
void CAdjustAcceleration::Process(int nArg)
{
    m_pSmoothVel->x = GetStep() * m_pVelDir->x + m_pSmoothVel->x;
    m_pSmoothVel->y = GetStep() * m_pVelDir->y + m_pSmoothVel->y;
    m_pSmoothVel->z = GetStep() * m_pVelDir->z + m_pSmoothVel->z;

    if (m_pNext != 0)
    {
        m_pNext->Process(nArg);
    }
}
