#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "planvm/agendaitem.h"
#include "planvm/csteertoward.h"
#include "graphics/meshbone.h"
#include "engine/objectstats.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "globals.h"
#include "math/vector.h"
#include "common/stub.h"
#include <cmath>
#include "engine/arenastate.h"
#include "physics/collisionellipsoid.h"
#include "common/pooledownerroot.h"
#include "engine/messageids.h"

// Aims six frames ahead of the target, and hands over to the target's matrix within six units.

// Arriving on slots zero to five is a pass to that player; anywhere else is a goal.

// The bone whose world transform the homing aim tracks.
constexpr auto kTargetBone = 15;

// FUNCTION: HYPERX 0x0040f180
void CSteerToward::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CSteerToward;
    m_bReady = 0;

    m_pPosition        = &pOwner->GetPhysicsBody()->pos;
    m_pTo              = &pOwner->GetPhysicsBody()->prevPos;
    m_pStep            = &pOwner->GetPhysicsBody()->vel;
    m_pVelDir          = &pOwner->GetPhysicsBody()->velDir;
    auto* const pFifth = &pOwner->GetPhysicsBody()->flSpeed;

    m_pOwner            = pOwner;
    m_pflSpeed          = pFifth;
    m_pPlayer           = pOwner->GetPhysicsBody()->pPlayer;
    m_pNetTarget        = &pOwner->GetPhysicsBody()->netTarget;
    m_pNetTargetPending = &pOwner->GetPhysicsBody()->bNetTargetPending;
    m_pPrevTick         = &pOwner->GetPhysicsBody()->nPrevTick;
}

// FUNCTION: HYPERX 0x0040f220
void CSteerToward::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext             = nullptr;
        m_pOwner            = nullptr;
        m_pPosition         = nullptr;
        m_pTo               = nullptr;
        m_pStep             = nullptr;
        m_pVelDir           = nullptr;
        m_pflSpeed          = nullptr;
        m_pTarget           = nullptr;
        m_pPlayer           = nullptr;
        m_pNetTarget        = nullptr;
        m_pNetTargetPending = nullptr;
        m_bReady            = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x0040f260
void CSteerToward::Process(int nArg)
{
    // The side vector's y is never written or read.
    Vec3  vSide;
    Vec3  vLead;
    float flPartial;

    vSide.x = g_flZeroAgendaItem;
    vSide.z = g_flZeroAgendaItem;

    vLead.x = g_flZeroAgendaItem;
    vLead.y = g_flZeroAgendaItem;
    vLead.z = g_flZeroAgendaItem;

    auto* pBody = m_pTarget->GetPhysicsBody();

    auto vOffset = pBody->pos;

    pBody = m_pOwner->GetPhysicsBody();

    auto vSelf = pBody->pos;

    pBody = m_pTarget->GetPhysicsBody();

    Vec3::ScaleTo(&pBody->velDir, 6.0f, &vLead);

    vOffset.x = (vLead.x - vSelf.x) + vOffset.x;
    vOffset.y = (vLead.y - vSelf.y) + vOffset.y;
    vOffset.z = (vLead.z - vSelf.z) + vOffset.z;

    if (sqrt(static_cast<double>(vOffset.x * vOffset.x + vOffset.y * vOffset.y + vOffset.z * vOffset.z)) < g_dHomingSix)
    {
        const auto nKind = m_pTarget->GetSlot();

        if (nKind < kPlayerObjectCount)
        {
            auto* const pPos = &m_pOwner->GetPhysicsBody()->pos;

            auto* const pMatrix = &m_pTarget->m_pBones[kTargetBone].m_world;

            flPartial = vSide.z * pMatrix->m_a[0][2] + vSide.x * pMatrix->m_a[0][0];

            pPos->x = flPartial + pMatrix->GetElement(0, 1) * 0.25f + pMatrix->m_a[0][3];

            flPartial = vSide.z * pMatrix->m_a[1][2] + vSide.x * pMatrix->m_a[1][0];

            pPos->y = flPartial + pMatrix->GetElement(1, 1) * 0.25f + pMatrix->m_a[1][3];

            flPartial = vSide.z * pMatrix->m_a[2][2] + vSide.x * pMatrix->m_a[2][0];

            pPos->z = flPartial + pMatrix->GetElement(2, 1) * 0.25f + pMatrix->m_a[2][3];

            pBody = m_pTarget->GetPhysicsBody();

            *m_pStep = pBody->vel;

            pBody = m_pTarget->GetPhysicsBody();

            *m_pVelDir = pBody->velDir;

            pBody = m_pTarget->GetPhysicsBody();

            *m_pflSpeed = pBody->flSpeed;

            m_pOwner->PostSceneEvent(kMsgPassReceived, m_pTarget->GetSlot());
        }
        else
        {
            m_pOwner->PostSceneEvent(kMsgGoalReceived, 0);
        }

        if (m_pNext != nullptr)
        {
            m_pNext->Process(nArg);
        }
    }
    else
    {
        Math::Vec3Normalize(&vOffset);

        Vec3::ScaleTo(&vOffset, *m_pflSpeed, m_pStep);

        m_pPosition->x = m_pStep->x + m_pPosition->x;
        m_pPosition->y = m_pStep->y + m_pPosition->y;
        m_pPosition->z = m_pStep->z + m_pPosition->z;
    }

    auto* const pStep = m_pStep;

    auto vWork = *pStep;

    Vec3::ScaleTo(pStep, g_pGameEngine->m_clock.GetInvDelta(), &vWork);

    pBody = m_pOwner->GetPhysicsBody();

    pBody->step = vWork;

    if (m_pNext != nullptr)
    {
        m_pNext->Process(nArg);
    }
}

#define GRAVITY_SCALE 16.0f

#define SPEED_EPSILON 0.0001

// The same integrator as CProjectAndBounce::IntegrateSpringAndGravity, over different field offsets.
// FUNCTION: HYPERX 0x0040f500
void CSteerToward::IntegrateSpringAndGravity()
{
    Vec3 vPull;

    const Vec3* const pPos = m_pPosition;

    vPull.x = m_vTarget.x - pPos->x;
    vPull.y = m_vTarget.y - pPos->y;
    vPull.z = m_vTarget.z - pPos->z;

    vPull.ScaleInPlace(g_pGameEngine->m_clock.GetDeltaSquared());

    auto* pVel = m_pStep;

    pVel->x = vPull.x + pVel->x;
    pVel->y = vPull.y + pVel->y;
    pVel->z = vPull.z + pVel->z;

    m_pPosition->x = m_pStep->x + m_pPosition->x;

    m_pPosition->y = m_pStep->y + m_pPosition->y;

    m_pStep->z -= g_pGameEngine->m_clock.flDeltaSquared * GRAVITY_SCALE;

    m_pPosition->z = m_pStep->z + m_pPosition->z;

    pVel = m_pStep;

    const auto flZ = pVel->z;
    const auto flY = pVel->y;
    const auto flX = pVel->x;

    *m_pflSpeed = static_cast<float>(sqrt(static_cast<double>(flX * flX + flY * flY + flZ * flZ)));

    if (*m_pflSpeed > SPEED_EPSILON)
    {
        Vec3::ScaleTo(m_pStep,
                      g_flOneAgendaItem / GetSpeed(),
                      m_pVelDir);
    }
}

inline void CSteerToward::AddStep(Vec3* pTarget, const Vec3* pStep)
{
    pTarget->x += pStep->x;
    pTarget->y = pStep->y + pTarget->y;
}

// Kind three's release step: seed the flight once, integrate, set the normal.
// FUNCTION: HYPERX 0x0040f600
void CSteerToward::Release(int nArg)
{
    PhysicsBody* pBody;
    Vec3         vScratch;

    const Vec3* const pFrom = m_pPosition;
    auto* const       pTo   = m_pTo;

    pTo->x = pFrom->x;
    pTo->y = pFrom->y;
    pTo->z = pFrom->z;

    auto* pFlag = m_pNetTargetPending;

    if (*pFlag != 0)
    {
        *pFlag = 0;

        m_vTarget.x = m_pNetTarget->x;
        m_vTarget.y = m_pNetTarget->y;
        m_vTarget.z = m_pNetTarget->z;

        const auto flDt = g_pGameEngine->m_clock.flDelta;

        pBody = m_pOwner->GetPhysicsBody();

        Vec3::ScaleTo(&pBody->step,
                      flDt,
                      m_pStep);
    }
    else
    {
        pBody = m_pOwner->GetPhysicsBody();

        AddStep(&m_vTarget, &pBody->step);

        m_pStep->z -= g_pGameEngine->m_clock.flDeltaSquared * GRAVITY_SCALE;

        m_vTarget.z = m_pStep->z + m_vTarget.z;
    }

    IntegrateSpringAndGravity();

    auto vPoint = *m_pOwner->GetWorldPos(&vScratch);

    const Vec3* pNormal = g_arenaEllipsoid.NormalAtPoint(&vPoint);

    if (pNormal != nullptr)
    {
        auto* const pSurfNormal = &m_pOwner->GetPhysicsBody()->surfNormal;

        *pSurfNormal = *pNormal;

        m_pOwner->GetPhysicsBody()->pShell = &g_arenaEllipsoid;
    }

    auto* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Release(nArg);
    }
}
