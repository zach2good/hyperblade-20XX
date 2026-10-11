// A tracked bone's frame displacement becomes the owner's velocity and, past 0.001, its heading.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "math/transformobject.h"
#include "devicector.h"
#include "teamblock.h"
#include "graphics/infobarradar.h"
#include "engine/objectstats.h"
#include "globals.h"
#include "planvm/agendaitem.h"
#include "engine/cbeattached.h"
#include "math/vector.h"
#include "gameengine.h"

#include <math.h>
#include "engine/engineobject.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x0040dbe0
void CBeAttached::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = 0;
    m_wKind  = Agenda_CBeAttached;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pVel     = &pOwner->GetPhysicsBody()->vel;
    m_pPos     = &pOwner->GetPhysicsBody()->pos;
    m_pPrevPos = &pOwner->GetPhysicsBody()->prevPos;
    m_pflSpeed = &pOwner->GetPhysicsBody()->flSpeed;
    m_pAttach  = pOwner->GetPhysicsBody()->pAttach;
}

// FUNCTION: HYPERX 0x0040dc40
void CBeAttached::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext    = 0;
        m_pOwner   = 0;
        m_pVel     = 0;
        m_pPos     = 0;
        m_pPrevPos = 0;
        m_pflSpeed = 0;
        m_pAttach  = 0;
        m_bReady   = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x0040dc80
void CBeAttached::Process(int nArg)
{
    TransformObject* pTransform;
    Vec3*            pOut;

    *m_pPrevPos = *m_pPos;

    PhysicsBody* pState = m_pOwner->GetPhysicsBody();
    AttachPak*   pBone  = pState->pAttach;

    m_pAttach = pBone;

    if (pBone != 0)
    {
        EngineObject* const pEntity = g_pGameEngine->m_ppSlots[pBone->nSlot];

        if ((pEntity->m_nFlags & ENGINEOBJECT_POSED) == 0)
        {
            pEntity->ApplyRenderSlotPose();
        }

        pBone        = m_pAttach;
        pOut         = m_pPos;
        Vec3 flLocal = pBone->vOffset;

        pTransform = pBone->pTransform;

        pOut->x = flLocal.x * pTransform->m_a[0][0] + flLocal.y * pTransform->m_a[0][1] + flLocal.z * pTransform->m_a[0][2] + pTransform->m_a[0][3];
        pOut->y = flLocal.x * pTransform->m_a[1][0] + flLocal.y * pTransform->m_a[1][1] + flLocal.z * pTransform->m_a[1][2] + pTransform->m_a[1][3];
        pOut->z = flLocal.x * pTransform->m_a[2][0] + flLocal.y * pTransform->m_a[2][1] + flLocal.z * pTransform->m_a[2][2] + pTransform->m_a[2][3];
    }

    m_pVel->x = m_pPos->x - m_pPrevPos->x;
    m_pVel->y = m_pPos->y - m_pPrevPos->y;
    m_pVel->z = m_pPos->z - m_pPrevPos->z;

    pOut = m_pVel;

    *m_pflSpeed = (float)sqrt(pOut->z * pOut->z + pOut->y * pOut->y + pOut->x * pOut->x);

    if (0.001 < *m_pflSpeed)
    {
        pState = m_pOwner->GetPhysicsBody();

        Vec3::ScaleTo(m_pVel, 1.0f / *m_pflSpeed, &pState->velDir);
    }

    pOut = m_pVel;

    Vec3 vVelocity = *pOut;

    Vec3::ScaleTo(pOut, g_pGameEngine->m_clock.GetInvDelta(), &vVelocity);

    pState = m_pOwner->GetPhysicsBody();

    pState->step = vVelocity;

    AgendaItem* const pNext = m_pNext;

    if (pNext != 0)
    {
        pNext->Process(nArg);
    }
}

// FUNCTION: HYPERX 0x0040de50
void CBeAttached::Release(int nArg)
{
    TransformObject* pTransform;
    Vec3*            pOut;

    *m_pPrevPos = *m_pPos;

    PhysicsBody* pState = m_pOwner->GetPhysicsBody();
    AttachPak*   pBone  = pState->pAttach;

    m_pAttach = pBone;

    if (pBone != 0)
    {
        EngineObject* const pEntity = g_pGameEngine->m_ppSlots[pBone->nSlot];

        if ((pEntity->m_nFlags & ENGINEOBJECT_POSED) == 0)
        {
            pEntity->ApplyRenderSlotPose();
        }

        pBone        = m_pAttach;
        pOut         = m_pPos;
        Vec3 flLocal = pBone->vOffset;

        pTransform = pBone->pTransform;

        pOut->x = flLocal.x * pTransform->m_a[0][0] + flLocal.y * pTransform->m_a[0][1] + flLocal.z * pTransform->m_a[0][2] + pTransform->m_a[0][3];
        pOut->y = flLocal.x * pTransform->m_a[1][0] + flLocal.y * pTransform->m_a[1][1] + flLocal.z * pTransform->m_a[1][2] + pTransform->m_a[1][3];
        pOut->z = flLocal.x * pTransform->m_a[2][0] + flLocal.y * pTransform->m_a[2][1] + flLocal.z * pTransform->m_a[2][2] + pTransform->m_a[2][3];
    }

    m_pVel->x = m_pPos->x - m_pPrevPos->x;
    m_pVel->y = m_pPos->y - m_pPrevPos->y;
    m_pVel->z = m_pPos->z - m_pPrevPos->z;

    pOut = m_pVel;

    *m_pflSpeed = (float)sqrt(pOut->z * pOut->z + pOut->y * pOut->y + pOut->x * pOut->x);

    if (0.001 < *m_pflSpeed)
    {
        pState = m_pOwner->GetPhysicsBody();

        Vec3::ScaleTo(m_pVel, 1.0f / *m_pflSpeed, &pState->velDir);
    }

    pOut = m_pVel;

    Vec3 vVelocity = *pOut;

    Vec3::ScaleTo(pOut, g_pGameEngine->m_clock.GetInvDelta(), &vVelocity);

    pState = m_pOwner->GetPhysicsBody();

    pState->step = vVelocity;

    AgendaItem* const pNext = m_pNext;

    if (pNext != 0)
    {
        pNext->Release(nArg);
    }
}
