// A forward vector pitched below the limit is clamped; the two differ only in the slot handed on.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "devicector.h"
#include "math/transformobject.h"
#include "globals.h"
#include "planvm/agendaitem.h"
#include "engine/csetattitude.h"
#include "math/vector.h"

#include <cmath>
#include "physics/physicsbody.h"
#include "app/gameapp.h"
#include "physics/collisionplane.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x00410400
void CSetAttitude::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CSetAttitude;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pVelDir     = &pOwner->GetPhysicsBody()->velDir;
    m_pFacing     = &pOwner->GetPhysicsBody()->flFacing;
    m_pTransform  = m_pOwner->GetLocalTransformPtr();
    m_pSurfNormal = &pOwner->GetPhysicsBody()->surfNormal;
}

// FUNCTION: HYPERX 0x00410460
void CSetAttitude::OnKind(short nSlot)
{
    PhysicsBody* pAnchorBlock = m_pOwner->GetPhysicsBody();
    Vec3* const  pAnchor      = &pAnchorBlock->pos;

    Vec3* const pTargetPos = g_pGameEngine->m_ppSlots[nSlot]->GetPositionPtr();

    // The anchor comes from the first answer and everything written goes into the second.
    PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

    m_pVelDir->x = pTargetPos->x - pAnchor->x;
    m_pVelDir->y = pTargetPos->y - pAnchor->y;
    m_pVelDir->z = pTargetPos->z - pAnchor->z;

    Math::Vec3Normalize(m_pVelDir);

    pBody->prevPos.x = pAnchor->x - m_pVelDir->x * pBody->flSpeed;
    pBody->prevPos.y = pAnchor->y - m_pVelDir->y * pBody->flSpeed;
    pBody->prevPos.z = pAnchor->z - m_pVelDir->z * pBody->flSpeed;

    pBody->vel.x = pAnchor->x - pBody->prevPos.x;
    pBody->vel.y = pAnchor->y - pBody->prevPos.y;
    pBody->vel.z = pAnchor->z - pBody->prevPos.z;

    pBody->flFacing = g_flZeroAgendaItem;

    Vec3* const pNormal = pBody->pShell->NormalAtPoint(pAnchor);

    if (pNormal == nullptr)
    {
        // STRING: HYPERX 0x004bb6c0
        g_gameApp.Fatal("BAD SURFACE NORMAL - InitOnSurface B.");
    }

    *m_pSurfNormal = *pNormal;
}

// FUNCTION: HYPERX 0x00410560
void CSetAttitude::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext       = nullptr;
        m_pOwner      = nullptr;
        m_pVelDir     = nullptr;
        m_pFacing     = nullptr;
        m_pTransform  = nullptr;
        m_pSurfNormal = nullptr;
        m_bLimitPitch = 0;
        m_bReady      = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x004105a0
void CSetAttitude::Process(int nArg)
{
    TransformObject* pTransform;
    Vec3             vForward;
    Vec3             vSide;
    Vec3             vHint;
    Vec3             vUp;

    const int nSlot = m_pOwner->GetSlot();

    if (nSlot < SLOT_BODY_PART || SLOT_BODY_PART_LAST < nSlot)
    {
        vForward = *m_pVelDir;

        m_pTransform->m_a[0][1] = m_pVelDir->x;
        m_pTransform->m_a[1][1] = m_pVelDir->y;
        m_pTransform->m_a[2][1] = m_pVelDir->z;
    }
    else
    {
        pTransform = m_pTransform;

        vForward.x = pTransform->m_a[0][1];
        vForward.y = pTransform->m_a[1][1];
        vForward.z = pTransform->m_a[2][1];
    }

    vHint.x = m_pSurfNormal->x;
    vHint.y = m_pSurfNormal->y;

    if (m_bLimitPitch != 0)
    {
        vHint.y *= 0.1f;

        if (m_pVelDir->z < -0.3f)
        {
            m_pVelDir->z = -0.3f;

            Math::Vec3Normalize(m_pVelDir);
        }
    }

    // The side is the forward crossed with the hint, whose z is taken as one.
    vSide.x = vForward.y - vHint.y * vForward.z;
    vSide.y = vHint.x * vForward.z - vForward.x;
    vSide.z = vHint.y * vForward.x - vHint.x * vForward.y;

    Math::Vec3Normalize(&vSide);

    m_pTransform->m_a[0][0] = vSide.x;
    m_pTransform->m_a[1][0] = vSide.y;
    m_pTransform->m_a[2][0] = vSide.z;

    vUp.x = vSide.y * vForward.z - vSide.z * vForward.y;
    vUp.y = vSide.z * vForward.x - vSide.x * vForward.z;
    vUp.z = vSide.x * vForward.y - vSide.y * vForward.x;

    Math::Vec3Normalize(&vUp);

    m_pTransform->m_a[0][2] = vUp.x;
    m_pTransform->m_a[1][2] = vUp.y;
    m_pTransform->m_a[2][2] = vUp.z;
    if (*m_pFacing != g_flZeroAgendaItem)
    {
        const float flAngle = *m_pFacing;

        pTransform = m_pTransform;

        pTransform->m_rotZ.flCos = static_cast<float>(cos(static_cast<double>(flAngle)));
        pTransform->m_rotZ.flSin = static_cast<float>(sin(static_cast<double>(flAngle)));

        pTransform->ApplyRotationZ();
    }

    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Process(nArg);
    }
}

// FUNCTION: HYPERX 0x00410760
void CSetAttitude::Release(int nArg)
{
    TransformObject* pTransform;
    Vec3             vForward;
    Vec3             vSide;
    Vec3             vHint;
    Vec3             vUp;

    const int nSlot = m_pOwner->GetSlot();

    if (nSlot < SLOT_BODY_PART || SLOT_BODY_PART_LAST < nSlot)
    {
        vForward = *m_pVelDir;

        m_pTransform->m_a[0][1] = m_pVelDir->x;
        m_pTransform->m_a[1][1] = m_pVelDir->y;
        m_pTransform->m_a[2][1] = m_pVelDir->z;
    }
    else
    {
        pTransform = m_pTransform;

        vForward.x = pTransform->m_a[0][1];
        vForward.y = pTransform->m_a[1][1];
        vForward.z = pTransform->m_a[2][1];
    }

    vHint.x = m_pSurfNormal->x;
    vHint.y = m_pSurfNormal->y;

    if (m_bLimitPitch != 0)
    {
        vHint.y *= 0.1f;

        if (m_pVelDir->z < -0.3f)
        {
            m_pVelDir->z = -0.3f;

            Math::Vec3Normalize(m_pVelDir);
        }
    }

    // The side is the forward crossed with the hint, whose z is taken as one.
    vSide.x = vForward.y - vHint.y * vForward.z;
    vSide.y = vHint.x * vForward.z - vForward.x;
    vSide.z = vHint.y * vForward.x - vHint.x * vForward.y;

    Math::Vec3Normalize(&vSide);

    m_pTransform->m_a[0][0] = vSide.x;
    m_pTransform->m_a[1][0] = vSide.y;
    m_pTransform->m_a[2][0] = vSide.z;

    vUp.x = vSide.y * vForward.z - vSide.z * vForward.y;
    vUp.y = vSide.z * vForward.x - vSide.x * vForward.z;
    vUp.z = vSide.x * vForward.y - vSide.y * vForward.x;

    Math::Vec3Normalize(&vUp);

    m_pTransform->m_a[0][2] = vUp.x;
    m_pTransform->m_a[1][2] = vUp.y;
    m_pTransform->m_a[2][2] = vUp.z;
    if (*m_pFacing != g_flZeroAgendaItem)
    {
        const float flAngle = *m_pFacing;

        pTransform = m_pTransform;

        pTransform->m_rotZ.flCos = static_cast<float>(cos(static_cast<double>(flAngle)));
        pTransform->m_rotZ.flSin = static_cast<float>(sin(static_cast<double>(flAngle)));

        pTransform->ApplyRotationZ();
    }

    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Release(nArg);
    }
}
