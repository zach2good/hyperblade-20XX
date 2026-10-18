// Kind one: steering toward the object it was bound against.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "engine/messageids.h"
#include "engine/devicector.h"
#include "engine/objectclasses.h"
#include "planvm/csteertotarget.h"
#include "physics/physicsbody.h"
#include "globals.h"
#include "engine/objectstats.h"
#include "math/vector.h"
#include "physics/banking.h"
#include "damagegate.h"
#include <cmath>
#include <cstdlib>
#include "engine/playerobject.h"
#include "common/pooledownerroot.h"
#include "engine/statids.h"

#define JITTER_MODULUS 0x32

#define RANGE_GATE_NEAR 2.0f
#define RANGE_GATE_FAR  50.0f
// The balance anything but a player steers with.
#define OBJECT_BALANCE  50.0f
#define RANGE_GATE_LEAD 25.0f

// 0x004b1648 is the double minus a thousandth and 0x004b15b8 the float.
#define BANK_DEADZONE_LOW  -0.001
#define BANK_DEADZONE_HIGH 0.001f

// FUNCTION: HYPERX 0x0040e020
void CSteerToTarget::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CSteerToTarget;
    m_bReady = 0;
    m_pOwner = pOwner;

    Vec3* const pWatched = &pOwner->GetPhysicsBody()->velDir;

    m_pTarget = nullptr;
    m_pAxis   = pWatched;
}

// FUNCTION: HYPERX 0x0040e060
void CSteerToTarget::SetTargetObjectByIndex(short nIndex)
{
    EngineObject* const pObject = g_pGameEngine->m_ppSlots[nIndex];

    m_cArrived = 0;
    m_pTarget  = pObject;
}

// FUNCTION: HYPERX 0x0040e080
void CSteerToTarget::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext    = nullptr;
        m_pOwner   = nullptr;
        m_pAxis    = nullptr;
        m_pTarget  = nullptr;
        m_cArrived = 0;
        m_bReady   = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x0040e0b0
void CSteerToTarget::Process(int nArg)
{
    float flForce;
    Vec3  vDelta;

    if (m_pOwner->GetSlot() <= kSlotDromer3)
    {
        flForce = ((PlayerObject*)m_pOwner)->m_stats.GetWeightedStat(kRatingBalance);
    }
    else if (m_pOwner->GetSlot() <= kGoalieSlotB)
    {
        flForce = ((PlayerObject*)m_pOwner)->m_stats.GetWeightedStat(kRatingBalance) * 10.0f;
    }
    else
    {
        if (m_pOwner->GetSlot() != kSlotRok)
        {
            m_pOwner->GetSlot();
        }

        flForce = OBJECT_BALANCE;
    }

    const float flClock = g_pGameEngine->m_clock.flDeltaSquared;
    const int   nJitter = rand() % JITTER_MODULUS;

    flForce = ((flForce * 0.1f + 1.5f) * flClock * 40.0f) * (static_cast<float>(nJitter) * 0.01f + 1.0f);

    Vec3* const pSelf     = m_pOwner->GetPositionPtr();
    Vec3* const pOther    = m_pTarget->GetPositionPtr();
    Vec3* const pOtherVel = &m_pTarget->GetPhysicsBody()->vel;

    vDelta.x = pOther->x - pSelf->x;
    vDelta.y = pOther->y - pSelf->y;
    vDelta.z = pOther->z - pSelf->z;

    PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

    const float flSpeed = pBody->flSpeed;

    Vec3 vFlat;

    Vec3 vHeading = pBody->velDir;
    vHeading.z    = 0.0f;
    vFlat.x       = vDelta.x;
    vFlat.y       = vDelta.y;
    vFlat.z       = 0.0f;

    Math::Vec3Normalize(&vHeading);
    Math::Vec3Normalize(&vFlat);

    if (Vec3::DotInline(&vHeading, &vFlat) < 0.75f)
    {
        PhysicsBody* const pTargetBody = m_pTarget->GetPhysicsBody();

        if (g_pGameEngine->m_clock.flInvDelta * pTargetBody->flSpeed > 5.0f)
        {
            float flGated = flSpeed;

            flGated *= 6.0f;

            const float flBrake =
                ((Vec3::DotInline(&vHeading, &vFlat) - 2.0f) * g_pGameEngine->m_clock.flDeltaSquared) * flGated;

            pBody->smoothVel.x = flBrake * pBody->vel.x;
            pBody->smoothVel.y = flBrake * pBody->vel.y;
            pBody->smoothVel.z = flBrake * pBody->vel.z;
        }
    }

    float flGate;

    PhysicsBody* const pTargetBody = m_pTarget->GetPhysicsBody();

    if (g_pGameEngine->m_clock.flInvDelta * pTargetBody->flSpeed > 3.0f)
    {
        flGate = RANGE_GATE_LEAD;

        float flLead = Vec3::DotInline(&vDelta, &vDelta) * 0.005f;

        if (flLead < 1.5f)
        {
            flLead = 0.0f;
        }
        else if (flLead > 5.0f)
        {
            flLead = 5.0f;
        }

        vDelta.x = flLead * pOtherVel->x + vDelta.x;
        vDelta.y = flLead * pOtherVel->y + vDelta.y;
        vDelta.z = flLead * pOtherVel->z + vDelta.z;
    }
    else
    {
        if (m_pTarget->GetSlot() == kSlotRok)
        {
            flGate = RANGE_GATE_NEAR;
        }
        else
        {
            flGate = RANGE_GATE_FAR;
        }
    }

    if (Vec3::DotInline(&vDelta, &vDelta) < flGate)
    {
        PhysicsBody* const pOwnBody = m_pOwner->GetPhysicsBody();

        pOwnBody->smoothVel.x -= pOwnBody->vel.x * 0.15f;
        pOwnBody->smoothVel.y -= pOwnBody->vel.y * 0.15f;
        pOwnBody->smoothVel.z -= pOwnBody->vel.z * 0.15f;

        if (m_cArrived == 0)
        {
            m_pOwner->PostSceneEvent(kMsgGotThere, 0);
            m_cArrived = 1;
        }
    }
    else
    {
        m_cArrived = 0;
    }

    Math::Vec3Normalize(&vDelta);

    const Vec3* const pAxis = m_pAxis;

    const float flLean = g_flOneAgendaItem - (vDelta.x * pAxis->x + (vDelta.y * pAxis->y + vDelta.z * pAxis->z));
    float       flCross;

    if ((flCross = vDelta.y * pAxis->x - vDelta.x * pAxis->y) < BANK_DEADZONE_LOW)
    {
        m_pOwner->GetPhysicsBody()->ApplyBankForce(0, static_cast<float>(fabs(static_cast<double>(flLean))) * flForce);
    }
    else if (flCross > BANK_DEADZONE_HIGH)
    {
        m_pOwner->GetPhysicsBody()->ApplyBankForce(1, static_cast<float>(fabs(static_cast<double>(flLean))) * flForce);
    }

    if (m_pNext != nullptr)
    {
        m_pNext->Process(nArg);
    }
}
