// Kind six: a run toward a point between two objects, aimed by a third.

#include "common/compat.h"
#include "math/mathops.h"
#include "planvm/agendaitemids.h"
#include "engine/messageids.h"
#include "common/behobjglidebetweenfacing.h"
#include "engine/gameengine.h"
#include "engine/devicector.h"
#include "engine/objectclasses.h"
#include "common/fieldat.h"
#include "planvm/agendaitem.h"
#include "planvm/csteerbetweenfacing.h"
#include "physics/physicsbody.h"
#include "globals.h"
#include "engine/objectstats.h"
#include "math/vector.h"
#include "physics/banking.h"
#include <math.h>
#include "engine/playerobject.h"
#include "engine/engineobject.h"
#include "common/pooledownerroot.h"
#include "engine/statids.h"

#define MSG_SPEED_BURST_ARG 0x96

#define BANK_DEADZONE_LOW  -0.001
#define BANK_DEADZONE_HIGH 0.001f

HYPERBLADE_FIELD_AT(CSteerBetweenFacing, m_pPath, 0x10);

// FUNCTION: HYPERX 0x0040eda0
void CSteerBetweenFacing::Bind(EngineObject* pOwner, GameBehaviorObject* pNode)
{
    m_pNext  = 0;
    m_wKind  = Agenda_CSteerBetweenFacing;
    m_bReady = 0;
    m_pOwner = pOwner;
    m_pPath  = (BehObj_GlideBetweenFacing*)pNode;

    m_pOrigin = &pOwner->GetPhysicsBody()->pos;
    m_pAxis   = &pOwner->GetPhysicsBody()->velDir;

    Vec3* const pNormal = &pOwner->GetPhysicsBody()->surfNormal;

    m_cArrived = 0;
    m_pNormal  = pNormal;
    m_flUnused = 100.0f;
}

// FUNCTION: HYPERX 0x0040ee00
void CSteerBetweenFacing::OnListChanged()
{
    if (m_bReady == 0)
    {
        PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

        pBody->flFacing = g_flZeroAgendaItem;

        m_pNext   = 0;
        m_pOwner  = 0;
        m_pOrigin = 0;
        m_pAxis   = 0;
        m_pNormal = 0;
        m_bReady  = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x0040ee50
void CSteerBetweenFacing::Process(int nArg)
{
    const float flForce = ((PlayerObject*)m_pOwner)->m_stats.GetWeightedStat(RATING_BALANCE) * 0.12f + 0.2f;

    Vec3 vDelta;
    Vec3 vAim;

    vDelta.x = g_flZeroAgendaItem;
    vDelta.y = g_flZeroAgendaItem;
    vDelta.z = g_flZeroAgendaItem;

    Vec3 vA = *m_pPath->m_pObjectA->GetPositionPtr();
    Vec3 vB = *m_pPath->m_pObjectB->GetPositionPtr();
    Vec3 vC = *m_pPath->m_pAimObject->GetPositionPtr();

    vAim.x = g_flZeroAgendaItem;
    vAim.y = g_flZeroAgendaItem;
    vAim.z = g_flZeroAgendaItem;

    Vec3 vDiff;

    vDiff.x = vA.x - vB.x;
    vDiff.y = vA.y - vB.y;
    vDiff.z = vA.z - vB.z;

    // Through its address, the step loads before the difference, as the client loads it.
    vA.x = m_pPath->GetBlendStep() * vDiff.x + vB.x;
    vA.y = m_pPath->GetBlendStep() * vDiff.y + vB.y;
    vA.z = m_pPath->GetBlendStep() * vDiff.z + vB.z;

    const Vec3* const pOrigin = m_pOrigin;

    vAim.x = vC.x - pOrigin->x;
    vAim.y = vC.y - pOrigin->y;
    vAim.z = vC.z - pOrigin->z;

    Math::Vec3Normalize(&vAim);

    const Vec3* const pAxis = m_pAxis;

    const float flAim = vAim.x * pAxis->x + (vAim.y * pAxis->y + vAim.z * pAxis->z);
    float       flFacing;

    if (vAim.y * pAxis->x - vAim.x * pAxis->y > 0.0f)
    {
        flFacing = (float)Math::Acos(flAim);
    }
    else
    {
        flFacing = -(float)Math::Acos(flAim);
    }

    m_pOwner->GetPhysicsBody()->flFacing = flFacing;

    const Vec3* const pPosition = m_pOrigin;

    vDelta.x = vA.x - pPosition->x;
    vDelta.y = (vA.y - pPosition->y) * 0.2f;
    vDelta.z = vA.z - pPosition->z;

    const float flRange = vDelta.Magnitude();

    if (flRange < 15.0)
    {
        const float flStop = g_pGameEngine->m_clock.GetDeltaSquared() * -10.0f;

        PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

        if (pBody->flSpeed > 0.1)
        {
            pBody->smoothVel.x = flStop * pBody->vel.x;
            pBody->smoothVel.y = flStop * pBody->vel.y;
            pBody->smoothVel.z = flStop * pBody->vel.z;
        }

        if (m_cArrived == 0)
        {
            m_pOwner->PostSceneEvent(MSG_GOT_THERE, 0);
            m_cArrived = 1;
        }
    }
    else
    {
        m_cArrived = 0;
    }

    if (flRange > 40.0)
    {
        m_pOwner->PostSceneEvent(MSG_SPEED_BURST, MSG_SPEED_BURST_ARG);
    }

    Math::Vec3Normalize(&vDelta);

    const Vec3* const pLeanAxis = m_pAxis;

    const float flLean = g_flOneAgendaItem - (vDelta.x * pLeanAxis->x + (vDelta.y * pLeanAxis->y + vDelta.z * pLeanAxis->z));

    float flCross;

    if ((flCross = vDelta.y * pLeanAxis->x - vDelta.x * pLeanAxis->y) < BANK_DEADZONE_LOW)
    {
        m_pOwner->GetPhysicsBody()->ApplyBankForce(0, (float)fabs(flLean) * flForce);
    }
    else if (flCross > BANK_DEADZONE_HIGH)
    {
        m_pOwner->GetPhysicsBody()->ApplyBankForce(1, (float)fabs(flLean) * flForce);
    }

    if (m_pNext != 0)
    {
        m_pNext->Process(nArg);
    }
}
