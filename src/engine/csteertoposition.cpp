#include "common/compat.h"
#include "math/mathops.h"
#include "planvm/agendaitemids.h"
#include "engine/messageids.h"
#include "physics/physicsbody.h"
#include "devicector.h"
#include "planvm/agendaitem.h"
#include "engine/csteertoposition.h"
#include "gameengine.h"
#include "globals.h"
#include "planvm/planvm.h"
#include "math/vector.h"
#include "app/gameapp.h"
#include "objectclasses.h"
#include "objectstats.h"
#include "physics/banking.h"
#include "planvm/damagegate.h"
#include "common/stub.h"
#include "playerobject.h"
#include <math.h>
#include <stdlib.h>
#include "physics/collisionplane.h"
#include "arenastate.h"
#include "physics/collisionellipsoid.h"
#include "common/pooledownerroot.h"
#include "engine/statids.h"

#define JITTER_MODULUS 0x32

#define BANK_DEADZONE_LOW -0.001

#define BANK_DEADZONE_HIGH 0.001f

// FUNCTION: HYPERX 0x0040e780
void CSteerToPosition::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = 0;
    m_wKind  = Agenda_CSteerToPosition;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pOrigin = &pOwner->GetPhysicsBody()->pos;
    m_pAxis   = &pOwner->GetPhysicsBody()->velDir;

    Vec3* const pSurfNormal = &pOwner->GetPhysicsBody()->surfNormal;

    m_pChase   = 0;
    m_pNormal  = pSurfNormal;
    m_cArrived = 0;
    m_flUnused = 100.0f;
}

// FUNCTION: HYPERX 0x0040e7e0
void CSteerToPosition::Tick(const Vec3* pTargetPos)
{
    m_vTarget.x = pTargetPos->x;
    m_vTarget.y = pTargetPos->y;

    CollisionShape* const pSurface = m_pOwner->GetPhysicsBody()->pShell;

    m_vTarget.z = pSurface->GetZ(m_vTarget.x,
                                 m_vTarget.y);
    m_cArrived  = 0;
}

// FUNCTION: HYPERX 0x0040e820
void CSteerToPosition::OnKind(short nSlot)
{
    m_pChase = g_pGameEngine->m_ppSlots[nSlot];

    Vec3* const pTargetPos = g_pGameEngine->m_ppSlots[nSlot]->GetPositionPtr();

    PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

    m_pAxis->x = pTargetPos->x - m_pOrigin->x;
    m_pAxis->y = pTargetPos->y - m_pOrigin->y;
    m_pAxis->z = pTargetPos->z - m_pOrigin->z;

    Math::Vec3Normalize(m_pAxis);

    pBody->prevPos.x = m_pOrigin->x - m_pAxis->x * pBody->flSpeed;
    pBody->prevPos.y = m_pOrigin->y - m_pAxis->y * pBody->flSpeed;
    pBody->prevPos.z = m_pOrigin->z - m_pAxis->z * pBody->flSpeed;

    pBody->vel.x = m_pOrigin->x - pBody->prevPos.x;
    pBody->vel.y = m_pOrigin->y - pBody->prevPos.y;
    pBody->vel.z = m_pOrigin->z - pBody->prevPos.z;

    pBody->flFacing = g_flZeroAgendaItem;

    Vec3* const pNormal = pBody->pShell->NormalAtPoint(m_pOrigin);

    if (pNormal == 0)
    {
        // STRING: HYPERX 0x004bb6c0
        g_gameApp.Fatal("BAD SURFACE NORMAL - InitOnSurface B.");
    }

    *m_pNormal = *pNormal;
}

// FUNCTION: HYPERX 0x0040e940
void CSteerToPosition::OnListChanged()
{
    if (m_bReady == 0)
    {
        PhysicsBody* const pState = m_pOwner->GetPhysicsBody();

        pState->flFacing = g_flZeroAgendaItem;

        m_pNext     = 0;
        m_pOwner    = 0;
        m_pOrigin   = 0;
        m_pAxis     = 0;
        m_pNormal   = 0;
        m_pChase    = 0;
        m_vTarget.x = m_vTarget.y = m_vTarget.z = g_flZeroAgendaItem;
        m_bReady                                = 1;
        m_cArrived                              = 0;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// Brakes hard inside fifteen units and sends the arrival message once.
// FUNCTION: HYPERX 0x0040e9b0
void CSteerToPosition::Process(int nArg)
{
    float flForce = (((PlayerObject*)m_pOwner)->m_stats.GetWeightedStat(RATING_BALANCE) * 0.1f + 1.5f) * g_pGameEngine->m_clock.GetDeltaSquared() * 40.0f;

    const int nJitter = rand() % JITTER_MODULUS;

    flForce = ((float)nJitter * 0.01f + 1.0f) * flForce;

    if (m_pChase != 0)
    {
        Vec3 vAim;

        Vec3* const       pChasePos = m_pChase->GetPositionPtr();
        const Vec3* const pOrigin   = m_pOrigin;

        vAim.x = pChasePos->x - pOrigin->x;
        vAim.y = pChasePos->y - pOrigin->y;
        vAim.z = pChasePos->z - pOrigin->z;

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

        Vec3 vAhead;

        vAhead.x = vAim.x * 0.25f + m_vTarget.x;
        vAhead.y = vAim.y * 0.25f + m_vTarget.y;

        g_arenaEllipsoid.GetZ(vAhead.x, vAhead.y);
    }

    Vec3 vDelta;

    const Vec3* const pOrigin = m_pOrigin;

    vDelta.x = m_vTarget.x - pOrigin->x;
    vDelta.y = m_vTarget.y - pOrigin->y;
    vDelta.z = m_vTarget.z - pOrigin->z;

    PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

    Vec3 vFlat;

    Vec3 vHeading = pBody->velDir;
    vHeading.z    = 0.0f;
    vFlat.x       = vDelta.x;
    vFlat.y       = vDelta.y;
    vFlat.z       = 0.0f;

    Math::Vec3Normalize(&vHeading);
    Math::Vec3Normalize(&vFlat);

    const float flSpeed = pBody->flSpeed;

    const float flDot = Vec3::DotInline(&vHeading, &vFlat);

    if (flDot < 0.6f)
    {
        const float flBrake = g_pGameEngine->m_clock.GetDeltaSquared() * (flDot - 2.0f) * (flSpeed * 4.0f);

        pBody->smoothVel.x = flBrake * pBody->vel.x;
        pBody->smoothVel.y = flBrake * pBody->vel.y;
        pBody->smoothVel.z = flBrake * pBody->vel.z;
    }
    else if (m_pOwner->GetSlot() > SLOT_DROMER3 || ((PlayerObject*)m_pOwner)->m_wClockStoppedSemaphore != 0)
    {
        if ((float)sqrt(Vec3::DotInline(&vDelta, &vDelta)) * g_pGameEngine->m_clock.flDelta < flSpeed + flSpeed)
        {
            const float flPush = g_pGameEngine->m_clock.GetDeltaSquared() * (flSpeed * 6.0f) * -1.0f;

            pBody->smoothVel.x = flPush * pBody->vel.x + pBody->smoothVel.x;
            pBody->smoothVel.y = flPush * pBody->vel.y + pBody->smoothVel.y;
            pBody->smoothVel.z = flPush * pBody->vel.z + pBody->smoothVel.z;
        }
    }

    if ((float)sqrt(Vec3::DotInline(&vDelta, &vDelta)) < 15.0)
    {
        const float flStop = g_pGameEngine->m_clock.flDeltaSquared * -15.0f;

        pBody->smoothVel.x = flStop * pBody->vel.x;
        pBody->smoothVel.y = flStop * pBody->vel.y;
        pBody->smoothVel.z = flStop * pBody->vel.z;

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

    Math::Vec3Normalize(&vDelta);

    const Vec3* const pAxis = m_pAxis;

    const float flLean = g_flOneAgendaItem - (vDelta.x * pAxis->x + (vDelta.y * pAxis->y + vDelta.z * pAxis->z));

    float flCross;

    if ((flCross = vDelta.y * pAxis->x - vDelta.x * pAxis->y) < BANK_DEADZONE_LOW)
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

// FUNCTION: HYPERX 0x004132c0
Vec3* CSteerToPosition::GetVector()
{
    return &m_vTarget;
}
