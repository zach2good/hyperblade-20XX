#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "planvm/agendaitem.h"
#include "engine/crelaxyaw.h"
#include "planvm/cglideonsurface.h"
#include "engine/devicector.h"
#include "engine/objectclasses.h"
#include "planvm.h"
#include "globals.h"
#include "engine/objectstats.h"
#include "math/vector.h"
#include "physics/collisionellipsoid.h"
#include "engine/gameengine.h"
#include "physics/banking.h"
#include "common/stub.h"
#include "net/session.h"
#include <cmath>
#include "app/gameapp.h"
#include "engine/playerobject.h"
#include "net/gamesession.h"
#include "physics/collisionplane.h"
#include "math/mathops.h"
#include "common/pooledownerroot.h"
#include "engine/statids.h"
#include "engine/messageids.h"

#define ROK_CARRIER_SPEED_SCALE 0.9f

// Thirty a second, in .data rather than .rdata.
// GLOBAL: HYPERX 0x004bb63c
extern float g_flBankFreeSpeed;

// FUNCTION: HYPERX 0x0040c0b0
void CGlideOnSurface::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CGlideOnSurface;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pflFriction       = &pOwner->GetPhysicsBody()->flFriction;
    m_pVel              = &pOwner->GetPhysicsBody()->vel;
    m_pVelDir           = &pOwner->GetPhysicsBody()->velDir;
    m_pVelDirPrev       = &pOwner->GetPhysicsBody()->velDirLatched;
    m_pAxis             = &pOwner->GetPhysicsBody()->axis;
    m_pPos              = &pOwner->GetPhysicsBody()->pos;
    m_pPosCopy          = &pOwner->GetPhysicsBody()->prevPos;
    m_pSurface          = pOwner->GetPhysicsBody()->pShell;
    m_pNormal           = &pOwner->GetPhysicsBody()->surfNormal;
    m_pBankScale        = &pOwner->GetPhysicsBody()->flGravDeltaVel;
    m_pSpeed            = &pOwner->GetPhysicsBody()->flSpeed;
    m_pBaseSpeed        = &pOwner->GetPhysicsBody()->flForwardSpeedDt;
    m_pLowAlt           = &pOwner->GetPhysicsBody()->flSteerScale;
    m_pBankForce        = &pOwner->GetPhysicsBody()->bankForce;
    m_pSmoothVel        = &pOwner->GetPhysicsBody()->smoothVel;
    m_pPlayer           = pOwner->GetPhysicsBody()->pPlayer;
    m_pTargetPos        = &pOwner->GetPhysicsBody()->netTarget;
    m_pNetTargetPending = &pOwner->GetPhysicsBody()->bNetTargetPending;
    m_pnPrevTick        = &pOwner->GetPhysicsBody()->nPrevTick;
    m_pMove             = &pOwner->GetPhysicsBody()->move;
    m_pnHeldBindings    = &pOwner->GetPhysicsBody()->nHeldBindings;

    m_nOnSurface = 0;
}

// FUNCTION: HYPERX 0x0040c200
void CGlideOnSurface::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext             = nullptr;
        m_pOwner            = nullptr;
        m_pflFriction       = nullptr;
        m_pVel              = nullptr;
        m_pVelDir           = nullptr;
        m_pVelDirPrev       = nullptr;
        m_pAxis             = nullptr;
        m_pPos              = nullptr;
        m_pPosCopy          = nullptr;
        m_pSurface          = nullptr;
        m_pNormal           = nullptr;
        m_pBankScale        = nullptr;
        m_pSpeed            = nullptr;
        m_pBaseSpeed        = nullptr;
        m_pLowAlt           = nullptr;
        m_pBankForce        = nullptr;
        m_pSmoothVel        = nullptr;
        m_pTargetPos        = nullptr;
        m_pNetTargetPending = nullptr;
        m_pMove             = nullptr;
        m_pnHeldBindings    = nullptr;
        m_nOnSurface        = 0;
        m_bReady            = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

namespace TerrainStep
{

static inline Vec3 Sum(const Vec3& vA, const Vec3& vB)
{
    Vec3 vOut;

    vOut.x = vA.x + vB.x;
    vOut.y = vA.y + vB.y;
    vOut.z = vA.z + vB.z;

    return vOut;
}

} // namespace TerrainStep

// FUNCTION: HYPERX 0x0040c270
void CGlideOnSurface::UpdateTerrainPhysics(int)
{
    PhysicsBody* pBody;
    Vec3*        pVelDir;
    Vec3*        pVelDirPrev;
    Vec3*        pTerm;
    Vec3*        pFound;
    float        flTarget;
    float        flDouble;
    float        flAlt;
    Vec3         vBank;
    Vec3         vSaved;
    Vec3         vCross;
    Vec3         vTarget;
    Vec3         vWorld;
    float        flSum;
    short        nCarrierSlot;
    char         bCarryingRok;

    vSaved = *m_pNormal;

    flDouble = *m_pflFriction * 2.0f;

    m_pSurface = m_pOwner->GetPhysicsBody()->pShell;

    *m_pPosCopy = *m_pPos;

    *m_pVelDirPrev = *m_pVelDir;

    if (*m_pflFriction >= 1.0)
    {
        m_pVelDir->x = m_pOwner->GetLocalTransformPtr()->m_a[0][1];
        m_pVelDir->y = m_pOwner->GetLocalTransformPtr()->m_a[1][1];
        m_pVelDir->z = m_pOwner->GetLocalTransformPtr()->m_a[2][1];

        Vec3::ScaleTo(m_pVelDir, 0.01f, m_pVel);

        m_pSmoothVel->z = 0.0f;
        m_pSmoothVel->y = 0.0f;
        m_pSmoothVel->x = 0.0f;
        m_pBankForce->z = 0.0f;
        m_pBankForce->y = 0.0f;
        m_pBankForce->x = 0.0f;

        if (m_nOnSurface != 0)
        {
            return;
        }
    }
    else
    {
        vTarget = TerrainStep::Sum(*m_pVel, *m_pPosCopy);

        m_pSurface->RayIntersection(m_pNormal, &vTarget, m_pPos);

        const Vec3 vDiff = m_pPos->Minus(m_pPosCopy);
        Vec3       vStep;

        vStep.x          = vDiff.x + m_pSmoothVel->x;
        vStep.y          = vDiff.y + m_pSmoothVel->y;
        vStep.z          = vDiff.z + m_pSmoothVel->z;
        const float flSx = vStep.x + m_pBankForce->x;
        const float flSy = vStep.y + m_pBankForce->y;
        const float flSz = vStep.z + m_pBankForce->z;

        vStep.x = flSx;
        vStep.y = flSy;
        vStep.z = flSz;

        Vec3 vOut = vStep;

        *m_pSpeed = static_cast<float>(sqrt(static_cast<double>(flSz * flSz + flSy * flSy + flSx * flSx)));

        float* pSpeed = m_pSpeed;

        if (g_pGameEngine->m_clock.flInvDelta * *pSpeed > 0.001f)
        {
            const float flLen = *pSpeed;
            Vec3* const pDst  = m_pVelDir;
            Vec3        vDir;

            vDir.x  = flSx * (g_flOneAgendaItem / flLen);
            vDir.y  = flSy * (g_flOneAgendaItem / flLen);
            vDir.z  = flSz * (g_flOneAgendaItem / flLen);
            pDst->x = vDir.x;
            pDst->y = vDir.y;
            pDst->z = vDir.z;
        }
        else
        {
            pVelDir = m_pVelDir;

            auto flLen = static_cast<float>(sqrt(static_cast<double>(pVelDir->x * pVelDir->x + pVelDir->y * pVelDir->y + pVelDir->z * pVelDir->z)));

            if (flLen == 0.0f)
            {
                pVelDirPrev = m_pVelDirPrev;

                flLen = static_cast<float>(sqrt(static_cast<double>(pVelDirPrev->x * pVelDirPrev->x + pVelDirPrev->y * pVelDirPrev->y + pVelDirPrev->z * pVelDirPrev->z)));

                if (flLen != 0.0f)
                {
                    *pVelDir = *pVelDirPrev;
                }
                else
                {
                    m_pPos->NormalizeTo(pVelDir);
                }
            }
        }

        pFound = m_pSurface->NormalAtPoint(m_pPos);

        if (pFound == nullptr)
        {
            // STRING: HYPERX 0x004bb6c0
            g_gameApp.Fatal("BAD SURFACE NORMAL - InitOnSurface B.");
        }

        *m_pNormal = *pFound;

        const Vec3 vNormal = *m_pNormal;

        if (m_pPos->z > g_flZeroAgendaItem)
        {
            m_pOwner->PostSceneEvent(kMsgFly, 0);
        }

        float* const pBankScale = m_pBankScale;
        vBank.x                 = vNormal.z * *pBankScale * vNormal.x;
        vBank.z                 = (-(vNormal.x * vNormal.x) - vNormal.y * vNormal.y) * *pBankScale;
        vBank.y                 = vNormal.y * *pBankScale;
        vBank.y *= vNormal.z;

        if (m_pOwner->GetSlot() < kPlayerObjectCount)
        {
            pVelDir = m_pVelDir;
            flAlt   = vBank.y * pVelDir->y + vBank.z * pVelDir->z + vBank.x * pVelDir->x;

            if (flAlt < 0.0)
            {
                Vec3::ScaleTo(pVelDir, flAlt, &vBank);
            }
            else
            {
                vBank.x = vBank.y = vBank.z = 0.0f;
            }
        }

        flDouble = g_pGameEngine->m_clock.GetDelta() * *m_pBankScale * vNormal.z * flDouble * 12.0f;

        int nSlot = m_pOwner->GetSlot();

        if (g_pGameEngine->m_clock.GetInvDelta() * *m_pSpeed < g_flBankFreeSpeed && (nSlot <= kGoalieSlotB || (nSlot >= kSlotBodyPart && nSlot <= kSlotBodyPartLast)))
        {
            pBody = m_pOwner->GetPhysicsBody();

            if (pBody->nHeldBindings == 0)
            {
                pVelDirPrev = m_pVelDirPrev;
                pVelDir     = m_pVelDir;

                flTarget = pVelDir->GetZ() * pVelDirPrev->z + pVelDir->GetY() * pVelDirPrev->y + pVelDirPrev->x * pVelDir->x;

                if (flTarget > g_flZeroAgendaItem)
                {
                    m_pOwner->PostSceneEvent(kMsgGlideToStop, 0);
                }
                else if (nSlot < kSlotBodyPart || nSlot > kSlotBodyPartLast)
                {
                    auto* const pRelax = (CRelaxYaw*)AllocAgendaItem(Agenda_CRelaxYaw);

                    PlanState* const pPlan = m_pOwner->GetPlanState();
                    pRelax->Bind(m_pOwner, pPlan->m_pCurrent);

                    pVelDirPrev = m_pVelDirPrev;
                    pVelDir     = m_pVelDir;

                    vCross.x = pVelDir->GetY() * pVelDirPrev->z - pVelDirPrev->GetY() * pVelDir->z;
                    vCross.y = pVelDir->z * pVelDirPrev->x - pVelDir->x * pVelDirPrev->z;
                    vCross.z = pVelDirPrev->GetY() * pVelDir->x - pVelDir->y * pVelDirPrev->x;

                    if (vCross.z > 0.0)
                    {
                        pRelax->m_flAngle = static_cast<float>(Math::Acos(flTarget));
                    }
                    else
                    {
                        pRelax->m_flAngle = -static_cast<float>(Math::Acos(flTarget));
                    }

                    pRelax->Activate();
                }
            }
        }
        else
        {
            vOut.x += vBank.x;
            vOut.y += vBank.y;
            vOut.z += vBank.z;
        }

        pTerm    = m_pSmoothVel;
        pTerm->z = g_flZeroAgendaItem;
        pTerm    = m_pSmoothVel;
        pTerm->y = g_flZeroAgendaItem;
        pTerm    = m_pSmoothVel;
        pTerm->x = g_flZeroAgendaItem;
        pTerm    = m_pBankForce;
        pTerm->z = g_flZeroAgendaItem;
        pTerm    = m_pBankForce;
        pTerm->y = g_flZeroAgendaItem;
        pTerm    = m_pBankForce;
        pTerm->x = g_flZeroAgendaItem;

        pBody = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody();

        AttachPak* const pRokAttach = pBody->pAttach;

        bCarryingRok = static_cast<char>(pRokAttach != nullptr && (nCarrierSlot = pRokAttach->nSlot) < kPlayerObjectCount && nCarrierSlot == m_pOwner->GetSlot());

        if (m_pPos->z > g_flZeroAgendaItem)
        {
            flAlt = g_flOneAgendaItem;
        }
        else
        {
            flAlt = *m_pLowAlt;
        }

        if (bCarryingRok != 0)
        {
            flTarget = ROK_CARRIER_SPEED_SCALE;
        }
        else
        {
            flTarget = g_flOneAgendaItem;
        }

        ObjectStats* const pStats = &((PlayerObject*)m_pOwner)->m_stats;

        if (pStats != nullptr)
        {
            flSum = pStats->GetWeightedStat(kRatingSpeed) * 0.05f + 0.65f;
        }
        else
        {
            flSum = g_flOneAgendaItem;
        }

        flTarget = *m_pBaseSpeed * flSum * flTarget * flAlt * g_flMaxSpeed;

        pSpeed = m_pSpeed;

        if (*pSpeed > flTarget)
        {
            *m_pVelDir = vOut;

            Math::Vec3Normalize(m_pVelDir);

            *pSpeed = flTarget;
            flTarget -= flDouble;

            vOut = m_pVelDir->Scaled(flTarget);
        }
        else if (flDouble <= *pSpeed)
        {
            const Vec3 vDrag = m_pVelDir->Scaled(flDouble);
            const Vec3 vNext = vOut.Minus(&vDrag);

            Vec3* const pVel = m_pVel;

            if (vNext.z * pVel->z + vNext.y * pVel->y + vNext.x * pVel->x < g_flZeroAgendaItem)
            {
                if (m_pOwner->GetSlot() <= kGoalieSlotB)
                {
                    pBody = m_pOwner->GetPhysicsBody();

                    if (pBody->nHeldBindings == 0)
                    {
                        m_pOwner->PostSceneEvent(kMsgGlideToStop, 0);

                        vOut.y = 0.0f;
                        vOut.x = 0.0f;
                        vOut.z = 0.0f;
                    }
                }
            }
            else
            {
                vOut = vNext;
            }
        }

        *m_pVel = vOut;

        Vec3 vPos;

        vWorld = *m_pOwner->GetWorldPos(&vPos);

        pFound = g_arenaEllipsoid.NormalAtPoint(&vWorld);

        if (pFound != nullptr)
        {
            pBody             = m_pOwner->GetPhysicsBody();
            pBody->surfNormal = *pFound;

            m_pOwner->GetPhysicsBody()->pShell = &g_arenaEllipsoid;

            vSaved.x = pFound->x;
            vSaved.y = pFound->y;
            vSaved.z = pFound->z;
        }

        pVelDir           = m_pVelDir;
        const float flDot = (vSaved.z * pVelDir->z + vSaved.y * pVelDir->y) + vSaved.x * pVelDir->x;

        const Vec3 vProj = vSaved.Scaled(flDot);

        pVelDir->x -= vProj.x;
        pVelDir->y -= vProj.y;
        pVelDir->z -= vProj.z;

        Math::Vec3Normalize(m_pVelDir);

        if (m_nOnSurface != 0)
        {
            return;
        }
    }

    Vec3::ScaleTo(m_pVel, g_pGameEngine->m_clock.GetInvDelta(), &vCross);

    pBody       = m_pOwner->GetPhysicsBody();
    pBody->step = vCross;
}

#define MS_PER_SECOND 1000.0f

// The session's lag counter is in milliseconds and the term it feeds is in seconds.
#define SESSION_LAG_SCALE 0.001f

// FUNCTION: HYPERX 0x0040cb00
void CGlideOnSurface::Process(int nArg)
{
    UpdateTerrainPhysics(nArg);

    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Process(nArg);
    }
}

// m_pNetTargetPending is the network correction; its z drift reads the x term back.
// FUNCTION: HYPERX 0x0040cb20
void CGlideOnSurface::Release(int nTick)
{
    PhysicsBody* pBody;

    if (m_nOnSurface != 0)
    {
        UpdateTerrainPhysics(nTick);

        if (m_pNext != nullptr)
        {
            m_pNext->Release(nTick);
        }

        return;
    }

    *m_pPosCopy = *m_pPos;

    if (*m_pNetTargetPending != 0)
    {
        Vec3 vStep;
        Vec3 vLag;

        *m_pNetTargetPending = 0;

        pBody = m_pOwner->GetPhysicsBody();
        vStep = pBody->step;

        const auto nElapsed =
            static_cast<unsigned int>(static_cast<long>(g_pGameEngine->m_clock.flDelta * MS_PER_SECOND));
        const unsigned int nTickBase = g_pGameEngine->m_clock.nPrevTickMs;
        const float        flDelta   = g_pGameEngine->m_clock.flDelta;

        pBody = m_pOwner->GetPhysicsBody();
        Vec3::ScaleTo(&pBody->step, flDelta, m_pVel);

        if (nElapsed > 0)
        {
            const float flDrift =
                (static_cast<float>(nTickBase + nElapsed) - static_cast<float>(*m_pnPrevTick)) / (g_pGameEngine->m_clock.flDelta * MS_PER_SECOND);

            vLag.x = flDrift * m_pVel->x;
            vLag.y = flDrift * m_pVel->y;
        }
        else
        {
            vLag.x = 0.0f;
            vLag.y = 0.0f;
        }

        float flLag;

        if (g_pGameSession != nullptr)
        {
            flLag = g_pGameSession->nLagMs * SESSION_LAG_SCALE;
        }
        else
        {
            flLag = 0.0f;
        }

        pBody = m_pOwner->GetPhysicsBody();
        Vec3::ScaleTo(&pBody->step, flLag, &vStep);

        m_pMove->target.x = vStep.x + m_pTargetPos->x + vLag.x;
        m_pMove->target.y = vStep.y + m_pTargetPos->y + vLag.y;
        m_pMove->target.z = vStep.z + m_pTargetPos->z + vLag.x;

        if (*m_pflFriction < 1.0)
        {
            m_pOwner->GetPhysicsBody()->IntegrateMotion();
        }
        else
        {
            Vec3* pDir;
            Vec3* pTarget;
            Vec3  vNormal;
            Vec3  vProj;

            vNormal = *m_pNormal;

            m_pVel->NormalizeTo(m_pVelDir);

            pDir = m_pVelDir;

            float flDot = vNormal.x * pDir->x + (vNormal.y * pDir->y + vNormal.z * pDir->z);

            vProj.x = flDot;
            vProj.x *= vNormal.x;
            vProj.y = flDot * vNormal.y;
            vProj.z = flDot * vNormal.z;

            pDir->x -= vProj.x;
            pDir->y -= vProj.y;
            pDir->z -= vProj.z;

            Math::Vec3Normalize(m_pVelDir);

            pTarget = m_pTargetPos;
            pBody   = m_pOwner->GetPhysicsBody();

            pBody->prevPos = pBody->pos;
            pBody->pos     = *pTarget;
        }
    }
    else
    {
        if (*m_pflFriction < 1.0)
        {
            m_pMove->target.x = m_pTargetPos->x + m_pVel->x;
            m_pMove->target.y = m_pTargetPos->y + m_pVel->y;
            m_pMove->target.z = m_pTargetPos->z + m_pVel->z;

            *m_pTargetPos = m_pMove->target;

            m_pOwner->GetPhysicsBody()->IntegrateMotion();
        }
        else
        {
            Vec3  vNormal;
            Vec3* pDir;
            Vec3  vProj;

            vNormal = *m_pNormal;

            m_pVel->NormalizeTo(m_pVelDir);

            pDir = m_pVelDir;

            const float flDot = vNormal.x * pDir->x + (vNormal.y * pDir->y + vNormal.z * pDir->z);

            vProj.x = flDot;
            vProj.x *= vNormal.x;
            vProj.y = flDot * vNormal.y;
            vProj.z = flDot * vNormal.z;

            pDir->x -= vProj.x;
            pDir->y -= vProj.y;
            pDir->z -= vProj.z;

            Math::Vec3Normalize(m_pVelDir);
        }
    }

    Vec3* const pNormal = g_arenaEllipsoid.NormalAtPoint(m_pPos);

    if (pNormal != nullptr)
    {
        pBody = m_pOwner->GetPhysicsBody();

        pBody->surfNormal = *pNormal;

        pBody = m_pOwner->GetPhysicsBody();

        pBody->pShell = &g_arenaEllipsoid;
    }

    if (m_pNext != nullptr)
    {
        m_pNext->Release(nTick);
    }
}

// Thirty a second: above it a terrain frame adds its bank vector whatever the owner's class.
float g_flBankFreeSpeed = 30.0f;
