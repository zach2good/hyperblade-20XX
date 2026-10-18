#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "engine/messageids.h"
#include "engine/gameengine.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "plannode.h"
#include "planvm/agendaitem.h"
#include "planvm/cbungeetoward.h"
#include "globals.h"
#include "planvm.h"
#include "common/stub.h"
#include "engine/objectclasses.h"
#include "math/vector.h"
#include "physics/banking.h"
#include <cmath>
#include "engine/engineobject.h"
#include "engine/arenastate.h"
#include "physics/collisionellipsoid.h"
#include "common/pooledownerroot.h"

// GLOBAL: HYPERX 0x004b17b0
extern float g_flTwoAgendaItem;

// An arity ladder: `> N` gates reading argument N.
#define ARG_SLOT 1
#define ARG_X    2
#define ARG_Y    3
constexpr auto kArgScalarA = 2;
constexpr auto kArgScalarB = 3;
constexpr auto kArgScalarC = 4;
constexpr auto kArgFlag    = 5;

constexpr auto kCountdownPoint  = 0x4b;
constexpr auto kCountdownObject = 0x19;

// A slot of minus one means a fixed point, written into marker table entry six, shared storage.
constexpr auto kPointMarker = 6;

#define LIMIT_DEFAULT 1000.0f

// FUNCTION: HYPERX 0x0040fee0
void CBungeeToward::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CBungeeToward;
    m_bReady = 0;
    m_pOwner = pOwner;

    Vec3* const pWatched = &pOwner->GetPhysicsBody()->smoothVel;

    m_pTarget      = nullptr;
    m_pAccum       = pWatched;
    m_pAnchor      = nullptr;
    m_flGain       = g_flOneAgendaItem;
    m_flRestLength = g_flZeroAgendaItem;
    m_flFarRange   = LIMIT_DEFAULT;
    m_flNearRange  = g_flZeroAgendaItem;
    m_flDotGate    = g_flHalfAgendaItem;
    m_nCountdown   = kCountdownObject;
    m_cAttached    = 0;
    m_cLeadTarget  = 0;
}

// FUNCTION: HYPERX 0x0040ff50
void CBungeeToward::BindWithNode(EngineObject* pObject, GameBehaviorObject*, PlanNode* pNode, int)
{
    short nSlot;

    m_pNext  = nullptr;
    m_wKind  = Agenda_CBungeeToward;
    m_bReady = 0;
    m_pOwner = pObject;

    m_pAccum = &pObject->GetPhysicsBody()->smoothVel;

    if (pNode->m_nArgCount > 1)
    {
        PlanWord* const pArgs = pNode->m_pArgs;

        if (pArgs[ARG_SLOT].IsPronoun())
        {
            nSlot = g_apPlanStatFns[pArgs[ARG_SLOT].wPronoun](pObject, pNode);
        }
        else
        {
            nSlot = static_cast<short>(pArgs[ARG_SLOT].n);
        }

        if (nSlot == kSlotNone)
        {
            Vec3* pPoint = g_apPositionMarkers[kPointMarker];

            m_pAnchor = pPoint;

            pPoint->x = static_cast<float>(static_cast<short>(pNode->m_pArgs[ARG_X].n));

            pPoint->y = static_cast<float>(static_cast<short>(pNode->m_pArgs[ARG_Y].n));

            pPoint = m_pAnchor;

            m_pAnchor->z = g_arenaEllipsoid.GetZ(pPoint->x, pPoint->y);

            m_flGain       = g_flTwoAgendaItem;
            m_flDotGate    = -g_flOneAgendaItem;
            m_flNearRange  = g_flZeroAgendaItem;
            m_nCountdown   = kCountdownPoint;
            m_flRestLength = g_flZeroAgendaItem;
            m_flFarRange   = LIMIT_DEFAULT;
            m_cAttached    = 0;
            m_cLeadTarget  = 0;
            m_pTarget      = nullptr;

            return;
        }

        m_pTarget = g_pGameEngine->m_ppSlots[nSlot];
    }
    else
    {
        m_pTarget = nullptr;
    }

    if (pNode->m_nArgCount > 2)
    {
        m_flGain = pNode->m_pArgs[kArgScalarA].fl;
    }
    else
    {
        m_flGain = g_flOneAgendaItem;
    }

    if (pNode->m_nArgCount > 3)
    {
        m_flDotGate = pNode->m_pArgs[kArgScalarB].fl;
    }
    else
    {
        m_flDotGate = g_flHalfAgendaItem;
    }

    if (pNode->m_nArgCount > 4)
    {
        m_flNearRange = pNode->m_pArgs[kArgScalarC].fl;
    }
    else
    {
        m_flNearRange = g_flZeroAgendaItem;
    }

    m_nCountdown = kCountdownObject;

    if (pNode->m_nArgCount > 5 && static_cast<short>(pNode->m_pArgs[kArgFlag].n) == 1)
    {
        m_cLeadTarget = 1;
    }
    else
    {
        m_cLeadTarget = 0;
    }

    m_pAnchor      = nullptr;
    m_flRestLength = g_flZeroAgendaItem;
    m_flFarRange   = LIMIT_DEFAULT;
    m_cAttached    = 0;
}

// The anchor is chased with a lead of four times the target's velocity when m_cLeadTarget is set.
#define LEAD_SCALE 4.0f

// FUNCTION: HYPERX 0x004100f0
void CBungeeToward::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext        = nullptr;
        m_pOwner       = nullptr;
        m_pAccum       = nullptr;
        m_pTarget      = nullptr;
        m_pAnchor      = nullptr;
        m_flGain       = g_flZeroAgendaItem;
        m_flRestLength = g_flZeroAgendaItem;
        m_flFarRange   = g_flZeroAgendaItem;
        m_flNearRange  = g_flZeroAgendaItem;
        m_flDotGate    = g_flZeroAgendaItem;
        m_cAttached    = 0;
        m_cLeadTarget  = 0;
        m_bReady       = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x00410160
void CBungeeToward::Process(int nArg)
{
    bool bHold = false;

    if (m_bReady != 0)
    {
        Detach();
        return;
    }

    Vec3* pAnchor = m_pAnchor;

    if (pAnchor == nullptr)
    {
        pAnchor = m_pTarget->GetPositionPtr();
    }

    Vec3* const pOwnPos = m_pOwner->GetPositionPtr();

    Vec3 vGap;

    vGap.x = pAnchor->x - pOwnPos->x;
    vGap.y = pAnchor->y - pOwnPos->y;
    vGap.z = pAnchor->z - pOwnPos->z;

    if (m_cLeadTarget != 0)
    {
        Vec3* const pVel = &m_pTarget->GetPhysicsBody()->vel;

        vGap.x = pVel->x * LEAD_SCALE + vGap.x;
        vGap.y = pVel->y * LEAD_SCALE + vGap.y;
        vGap.z = pVel->z * LEAD_SCALE + vGap.z;
    }

    float flLength = vGap.Magnitude();

    Math::Vec3Normalize(&vGap);

    const float flPull = (flLength - m_flRestLength) * m_flGain;

    m_pAccum->x = flPull * vGap.x + m_pAccum->x;
    m_pAccum->y = flPull * vGap.y + m_pAccum->y;
    m_pAccum->z = flPull * vGap.z + m_pAccum->z;

    PhysicsBody* pOwnBody;
    PhysicsBody* pTargetBody;

    if ((m_cAttached != 0 && flLength < m_flNearRange && (pOwnBody = m_pOwner->GetPhysicsBody(), pTargetBody = m_pTarget->GetPhysicsBody(), pOwnBody->flSpeed > pTargetBody->flSpeed + 0.1)) || flPull < 0.0)
    {
        pTargetBody = m_pTarget->GetPhysicsBody();
        pOwnBody    = m_pOwner->GetPhysicsBody();

        const float flRatio = pTargetBody->flSpeed / pOwnBody->flSpeed;

        PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

        pBody->vel.ScaleInPlace(flRatio);
        m_pOwner->PostSceneEvent(kMsgGotThere, 0);
    }

    if (m_bReady == 0 && flLength >= m_flNearRange && flLength <= m_flFarRange && m_nCountdown-- >= 0)
    {
        Vec3* const pHeading = &m_pOwner->GetPhysicsBody()->velDir;

        if (vGap.z * pHeading->z + vGap.y * pHeading->y + vGap.x * pHeading->x >= m_flDotGate)
        {
            AttachPak* const pCarrier = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;
            short            nCarrier;

            if (pCarrier == nullptr || (nCarrier = pCarrier->nSlot) >= kPlayerObjectCount || nCarrier != m_pOwner->GetSlot())
            {
                bHold = m_pTarget == nullptr || m_pTarget->GetPhysicsBody()->flFriction <= g_flHalfAgendaItem;
            }
        }
    }

    if (!bHold)
    {
        if (m_pTarget != nullptr)
        {
            m_pTarget->PostSceneEvent(kMsgYerReleased, 0);
        }

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
