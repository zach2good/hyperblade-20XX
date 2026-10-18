#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "planvm/agendaitem.h"
#include "planvm/cprojectandbounce.h"
#include "physics/collisionellipsoid.h"
#include "engine/objectstats.h"
#include "globals.h"
#include "math/vector.h"
#include "engine/devicector.h"
#include "engine/gameengine.h"
#include "common/stub.h"
#include <cmath>
#include "app/gameapp.h"
#include "physics/collisionshape.h"
#include "physics/collisionplane.h"
#include "planvmglobals.h"
#include "common/pooledownerroot.h"
#include "engine/messageids.h"

// Two; src/globals.cpp defines it.
extern float g_flTwoAgendaItem;

// FUNCTION: HYPERX 0x0040d3b0
void CProjectAndBounce::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CProjectAndBounce;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pVelocity = &pOwner->GetPhysicsBody()->vel;

    if (g_flFrameRateReference * g_pGameEngine->m_clock.GetDelta() < m_pVelocity->z)
    {
        m_pVelocity->z = g_flFrameRateReference / g_pGameEngine->m_clock.GetDelta();
    }

    m_pVelDir = &pOwner->GetPhysicsBody()->velDir;
    m_pSource = &pOwner->GetPhysicsBody()->pos;
    m_pDest   = &pOwner->GetPhysicsBody()->prevPos;

    m_vStep = *m_pSource;

    m_pSpeed            = &pOwner->GetPhysicsBody()->flSpeed;
    m_pShape            = pOwner->GetPhysicsBody()->pShell;
    m_pNormal           = &pOwner->GetPhysicsBody()->surfNormal;
    m_pPlayer           = pOwner->GetPhysicsBody()->pPlayer;
    m_pTarget           = &pOwner->GetPhysicsBody()->netTarget;
    m_pNetTargetPending = &pOwner->GetPhysicsBody()->bNetTargetPending;

    unsigned int* const pLast = &pOwner->GetPhysicsBody()->nPrevTick;

    m_nMode      = 0;
    m_pnPrevTick = pLast;
}

// FUNCTION: HYPERX 0x0040d4c0
void CProjectAndBounce::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext             = nullptr;
        m_wKind             = Agenda_CProjectAndBounce;
        m_pOwner            = nullptr;
        m_pVelocity         = nullptr;
        m_pVelDir           = nullptr;
        m_pSource           = nullptr;
        m_pDest             = nullptr;
        m_pSpeed            = nullptr;
        m_pShape            = nullptr;
        m_pNormal           = nullptr;
        m_pPlayer           = nullptr;
        m_pTarget           = nullptr;
        m_pNetTargetPending = nullptr;
        m_nMode             = 0;
        m_bReady            = 1;

        g_apAgendaItemPools[Agenda_CProjectAndBounce]->Insert(this);
    }
}

// One frame of a projectile riding the arena's collision ellipsoid; the argument is never read.
// FUNCTION: HYPERX 0x0040d510
void CProjectAndBounce::ProjectAndBounce(int)
{
    Vec3* pStep;
    Vec3* pClosest;
    Vec3* pNormal;
    Vec3  vFar;
    Vec3  vNear;
    float flSpan;
    float flFrac;
    float flCap;

    vNear.x = 0.0f;
    vNear.y = 0.0f;
    vNear.z = 0.0f;
    vFar.x  = 0.0f;
    vFar.y  = 0.0f;
    vFar.z  = 0.0f;

    m_pOwner->GetPhysicsBody()->IntegrateGravity();

    if (m_pShape->GetShapeKind() != SHAPE_KIND_ELLIPSOID)
    {
        m_pOwner->GetPhysicsBody()->pShell = &g_arenaEllipsoid;
        m_pShape                           = &g_arenaEllipsoid;
    }

    Vec3* const pPos   = m_pSource;
    auto* const pShape = (CollisionEllipsoid*)m_pShape;

    if (pPos->z < g_flZeroAgendaItem)
    {
        if (pShape->PointInside(pPos) == 0)
        {
            if (pShape->IntersectSegment(m_pDest, m_pSource, &vNear, &vFar) != 1)
            {
                Vec3 vStart;

                vStart.x = g_flZeroAgendaItem;
                vStart.y = g_flZeroAgendaItem;
                vStart.z = g_flZeroAgendaItem;

                pShape->IntersectSegment(&vStart, m_pSource, &vNear, &vFar);

                pClosest = Vec3::FindClosest(m_pSource, &vNear, &vFar);
            }
            else
            {
                pClosest = Vec3::FindClosest(m_pSource, &vNear, &vFar);
            }
            Vec3* pPosB = m_pSource;

            if (pClosest->z - pPosB->z > g_flZeroAgendaItem)
            {
                *pPosB = *pClosest;

                Vec3* const pPosZ = m_pSource;
                pPosZ->z          = pPosZ->z + 1.0f;
            }

            pNormal = m_pShape->NormalAtPoint(pClosest);

            if (pNormal == nullptr)
            {
                // STRING: HYPERX 0x004bb710
                g_gameApp.Fatal("BAD SURFACE NORMAL - CProjectAndBounce::Process().");
            }

            *m_pNormal = *pNormal;

            m_pOwner->GetPhysicsBody()->UpdateVelocityDirection();

            Vec3*       pPosC = m_pSource;
            Vec3* const pPrev = m_pDest;

            if ((flSpan = pPosC->x - pPrev->x) != g_flZeroAgendaItem)
            {
                flFrac = (float)fabs(static_cast<double>((pClosest->x - pPrev->x) / flSpan));
            }
            else
            {
                if ((flSpan = pPosC->y - pPrev->y) != g_flZeroAgendaItem)
                {
                    flFrac = (float)fabs(static_cast<double>((pClosest->y - pPrev->y) / flSpan));
                }
                else
                {
                    if ((flSpan = pPosC->z - pPrev->z) != g_flZeroAgendaItem)
                    {
                        flFrac = (float)fabs(static_cast<double>((pClosest->z - pPrev->z) / flSpan));
                    }
                    else
                    {
                        flFrac = g_flZeroAgendaItem;
                    }
                }
            }

            *pPosC = *pClosest;

            m_pOwner->GetPhysicsBody()->StepWithGravity(g_flOneAgendaItem - flFrac);

            if (m_pShape->PointInside(m_pSource) != 0 || m_pSource->z >= g_flZeroAgendaItem)
            {
                m_pOwner->PostSceneEvent(MSG_BOUNCE, 0);
            }
            else
            {
                m_pOwner->GetPhysicsBody()->smoothVel.z = g_flZeroAgendaItem;
                m_pOwner->GetPhysicsBody()->smoothVel.y = g_flZeroAgendaItem;
                m_pOwner->GetPhysicsBody()->smoothVel.x = g_flZeroAgendaItem;

                m_pOwner->PostSceneEvent(MSG_LANDED, 0);
            }
        }
    }
    else
    {
        const float flPy = pPos->y;

        if ((pShape->GetInvXX() * pPos->x * pPos->x) + pShape->GetInvYY() * flPy * flPy > 1.0)
        {
            Vec3 vBoundary;

            pShape->ComputeRadialBoundaryXY(pPos, &vBoundary);

            vBoundary.z = g_flZeroAgendaItem;

            pNormal = m_pShape->NormalAtPoint(&vBoundary);

            if (pNormal == nullptr)
            {
                // STRING: HYPERX 0x004bb6e8
                g_gameApp.Fatal("BAD SURFACE NORMAL - ProjectAndBounce.");
            }

            *m_pNormal = *pNormal;

            const float flSavedVelZ = m_pVelocity->z;

            m_pOwner->GetPhysicsBody()->UpdateVelocityDirection();

            m_pSource->x   = vBoundary.x;
            m_pSource->y   = vBoundary.y;
            m_pVelocity->z = flSavedVelZ;
        }
    }

    Vec3* pVel = m_pVelocity;

    const float flVz = pVel->z;
    const float flVy = pVel->y;
    const float flVx = pVel->x;

    *m_pSpeed = (float)sqrt(static_cast<double>(flVx * flVx + flVy * flVy + flVz * flVz));

    if ((flCap = g_flTwoAgendaItem * m_pOwner->GetPhysicsBody()->flForwardSpeedDt) < *m_pSpeed)
    {
        *m_pSpeed = flCap;

        m_pVelocity->x = GetSpeed() * m_pVelDir->x;
        m_pVelocity->y = GetSpeed() * m_pVelDir->y;
        m_pVelocity->z = GetSpeed() * m_pVelDir->z;
    }

    if (m_nMode == 0)
    {
        Vec3 vStep;

        pVel = m_pVelocity;

        vStep = *pVel;

        Vec3::ScaleTo(pVel, g_pGameEngine->m_clock.GetInvDelta(), &vStep);

        pStep = &m_pOwner->GetPhysicsBody()->step;

        pStep->x = vStep.x;
        pStep->y = vStep.y;
        pStep->z = vStep.z;
    }

    pNormal = g_arenaEllipsoid.NormalAtPoint(m_pSource);

    if (pNormal != nullptr)
    {
        pStep = &m_pOwner->GetPhysicsBody()->surfNormal;

        *pStep = *pNormal;

        m_pOwner->GetPhysicsBody()->pShell = &g_arenaEllipsoid;
    }
}

#define GRAVITY_SCALE 16.0f

#define SPEED_EPSILON 0.0001

// FUNCTION: HYPERX 0x0040d960
void CProjectAndBounce::Process(int nArg)
{
    ProjectAndBounce(nArg);

    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Process(nArg);
    }
}

// The vertical axis gets this frame's gravity before its step; the other two do not.
// FUNCTION: HYPERX 0x0040d980
void CProjectAndBounce::IntegrateSpringAndGravity()
{
    Vec3 vPull;

    Vec3* const pPos = m_pSource;

    vPull.x = m_vStep.x - pPos->x;
    vPull.y = m_vStep.y - pPos->y;
    vPull.z = m_vStep.z - pPos->z;

    vPull.ScaleInPlace(g_pGameEngine->m_clock.GetDeltaSquared());

    Vec3* pVel = m_pVelocity;

    pVel->x = vPull.x + pVel->x;
    pVel->y = vPull.y + pVel->y;
    pVel->z = vPull.z + pVel->z;

    m_pSource->x = m_pVelocity->x + m_pSource->x;

    m_pSource->y = m_pVelocity->y + m_pSource->y;

    m_pVelocity->z -= g_pGameEngine->m_clock.flDeltaSquared * GRAVITY_SCALE;

    m_pSource->z = m_pVelocity->z + m_pSource->z;

    pVel = m_pVelocity;

    const float flZ = pVel->z;
    const float flY = pVel->y;
    const float flX = pVel->x;

    *m_pSpeed = (float)sqrt(static_cast<double>(flX * flX + flY * flY + flZ * flZ));

    if (*m_pSpeed > SPEED_EPSILON)
    {
        Vec3::ScaleTo(m_pVelocity,
                      g_flOneAgendaItem / GetSpeed(),
                      m_pVelDir);
    }
}

inline void CProjectAndBounce::AddStep(const Vec3* pStep)
{
    m_vStep.x += pStep->x;
    m_vStep.y = pStep->y + m_vStep.y;
}

// Mode zero is the whole integrator; any other mode hands the frame to the projecting worker.
// FUNCTION: HYPERX 0x0040da80
void CProjectAndBounce::Release(int nMessage)
{
    PhysicsBody* pBody;
    float        flDelta;
    Vec3         scratch;

    if (m_nMode != 0)
    {
        ProjectAndBounce(nMessage);

        if (m_pNext != nullptr)
        {
            m_pNext->Release(nMessage);
        }

        return;
    }

    *m_pDest = *m_pSource;

    if (*m_pNetTargetPending != 0)
    {
        *m_pNetTargetPending = 0;

        m_vStep.x = m_pTarget->x;
        m_vStep.y = m_pTarget->y;
        m_vStep.z = m_pTarget->z;

        flDelta = g_pGameEngine->m_clock.flDelta;

        pBody = m_pOwner->GetPhysicsBody();

        Vec3::ScaleTo(&pBody->step, flDelta, m_pVelocity);
    }
    else
    {
        pBody = m_pOwner->GetPhysicsBody();

        AddStep(&pBody->step);

        m_pVelocity->z -= g_pGameEngine->m_clock.flDeltaSquared * GRAVITY_SCALE;

        m_vStep.z = m_pVelocity->z + m_vStep.z;
    }

    IntegrateSpringAndGravity();

    Vec3 point = *m_pOwner->GetWorldPos(&scratch);

    Vec3* const pNormal = g_arenaEllipsoid.NormalAtPoint(&point);

    if (pNormal != nullptr)
    {
        pBody = m_pOwner->GetPhysicsBody();

        pBody->surfNormal = *pNormal;

        pBody = m_pOwner->GetPhysicsBody();

        pBody->pShell = &g_arenaEllipsoid;
    }

    if (m_pNext != nullptr)
    {
        m_pNext->Release(nMessage);
    }
}
