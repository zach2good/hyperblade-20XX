// The range gate adds the truncated horizontal components as integers, not the gap's length.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "engine/objectclasses.h"
#include "planvm/agendaitem.h"
#include "planvm/cavoidobjects.h"
#include "globals.h"
#include "planvmglobals.h"
#include "math/vector.h"
#include "physics/banking.h"
#include <math.h>
#include <stdlib.h>
#include "engine/playerobject.h"
#include "engine/engineobject.h"
#include "plannode.h"
#include "engine/gameengine.h"
#include "common/pooledownerroot.h"
#include "engine/messageids.h"
#include "app/gameapp.h"

// How many frames on the wrong side the counter tolerates before the message goes out.
#define WRONG_SIDE_LIMIT 0x14

// Two; src/globals.cpp defines it.
extern float g_flTwoAgendaItem;

// Four bytes between two pool counts in .data, read by this instruction alone.
// GLOBAL: HYPERX 0x004bb654
extern float g_flSwerveMinSpeed;

// Frames running on the wrong side of the first target; only this function uses it.
// GLOBAL: HYPERX 0x004cb4d8
extern short g_nWrongSideFrames;

// FUNCTION: HYPERX 0x0040f830
void CAvoidObjects::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = 0;
    m_wKind  = Agenda_CAvoidObjects;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pLine  = &pOwner->GetPhysicsBody()->velDir;
    m_pAccum = &pOwner->GetPhysicsBody()->smoothVel;

    float* const pThird = &pOwner->GetPhysicsBody()->flAccelDt;

    m_cReleaseOnRange = 0;
    m_pLateral        = pThird;
    m_flRange         = 80.0f;
    m_flStrength      = -6.5f;
    m_pSwerveTarget   = 0;
    m_pDrawTarget     = 0;
}

// FUNCTION: HYPERX 0x0040f890
void CAvoidObjects::BindWithNode(EngineObject* pObject, GameBehaviorObject*, PlanNode* pNode, int nArg)
{
    const short nFallbackIndex = (short)nArg;

    m_pNext  = 0;
    m_wKind  = Agenda_CAvoidObjects;
    m_bReady = 0;
    m_pOwner = pObject;

    m_pLine    = &pObject->GetPhysicsBody()->velDir;
    m_pAccum   = &pObject->GetPhysicsBody()->smoothVel;
    m_pLateral = &pObject->GetPhysicsBody()->flAccelDt;

    if (pNode->m_nArgCount == 3)
    {
        m_pSwerveTarget = g_pGameEngine->m_ppSlots[(short)pNode->m_pArgs[1].n];
        m_pDrawTarget   = g_pGameEngine->m_ppSlots[(short)pNode->m_pArgs[2].n];
    }

    if (pNode->m_nArgCount == 5)
    {
        if (pNode->m_pArgs[1].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            m_pSwerveTarget = g_pGameEngine->m_ppSlots[nFallbackIndex];
        }
        else
        {
            m_pSwerveTarget = g_pGameEngine->m_ppSlots[(short)pNode->m_pArgs[1].n];
        }

        m_pDrawTarget     = 0;
        m_cReleaseOnRange = (char)pNode->m_pArgs[2].n;
        m_flRange         = pNode->m_pArgs[3].fl;
        m_flStrength      = pNode->m_pArgs[4].fl;
    }
}

// FUNCTION: HYPERX 0x0040f970
void CAvoidObjects::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext           = 0;
        m_pOwner          = 0;
        m_pLine           = 0;
        m_pAccum          = 0;
        m_pLateral        = 0;
        m_pSwerveTarget   = 0;
        m_pDrawTarget     = 0;
        m_bReady          = 1;
        m_cReleaseOnRange = 0;
        m_flRange         = g_flZeroAgendaItem;
        m_flStrength      = g_flZeroAgendaItem;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

inline short CAvoidObjects::SignTowardGoal(short nGoalSlot)
{
    if (nGoalSlot != SLOT_NGOAL)
    {
        return -1;
    }

    return 1;
}

// FUNCTION: HYPERX 0x0040f9c0
void CAvoidObjects::Process(int nArg)
{
    Vec3 vGap;

    Vec3 vAxis;

    vAxis.x = g_flZeroAgendaItem;
    vAxis.y = g_flZeroAgendaItem;
    vAxis.z = g_flOneAgendaItem;

    float flLength = g_flZeroAgendaItem;

    float flRoot;
    int   bPush = 1;

    const short nSign = SignTowardGoal(((PlayerObject*)m_pOwner)->m_wTargetGoalSlot);

    PhysicsBody* const pOwnBody = m_pOwner->GetPhysicsBody();

    if (pOwnBody->flFriction >= 0.8f || g_pGameEngine->m_clock.flInvDelta * pOwnBody->flSpeed < g_flSwerveMinSpeed)
    {
        bPush = 0;
    }

    // Zero where the first block bails; the original read stack garbage in the wrong-side test.
    float flDeltaY = 0.0f;
    float flDeltaX;

    if (m_pOwner->GetDistIfFacing(m_pSwerveTarget, g_flZeroAgendaItem) != 0.0f)
    {
        float flScale;

        Vec3* pTargetPos = m_pSwerveTarget->GetPositionPtr();
        Vec3* pOwnPos    = m_pOwner->GetPositionPtr();

        flDeltaX = pTargetPos->x - pOwnPos->x;

        flDeltaY = pTargetPos->y - pOwnPos->y;

        if ((float)(abs((int)flDeltaY) + abs((int)flDeltaX)) < m_flRange)
        {
            vGap.x = flDeltaX;
            vGap.y = flDeltaY;
            vGap.z = pTargetPos->z - pOwnPos->z;

            flRoot = (float)sqrt(vGap.z * vGap.z + flDeltaY * flDeltaY + flDeltaX * flDeltaX);

            flLength = flRoot;

            Math::Vec3Normalize(&vGap);

            Vec3 vSide;

            Vec3* const pHeading = &m_pOwner->GetPhysicsBody()->velDir;

            if (vGap.y * pHeading->x - vGap.x * pHeading->y > 0.0f)
            {
                Vec3* const pRight = &m_pOwner->GetPhysicsBody()->velDir;

                vSide.x = vAxis.y * pRight->z - vAxis.z * pRight->y;
                vSide.y = vAxis.z * pRight->x - vAxis.x * pRight->z;
                vSide.z = vAxis.x * pRight->y - vAxis.y * pRight->x;
            }
            else
            {
                Vec3* const pLeft = &m_pOwner->GetPhysicsBody()->velDir;

                vSide.x = vAxis.z * pLeft->y - vAxis.y * pLeft->z;
                vSide.y = vAxis.x * pLeft->z - vAxis.z * pLeft->x;
                vSide.z = vAxis.y * pLeft->x - vAxis.x * pLeft->y;
            }

            if (flLength > g_flHalfAgendaItem)
            {
                flScale = m_flStrength / flLength;
            }
            else
            {
                flScale = g_flTwoAgendaItem * m_flStrength;
            }

            flScale = g_pGameEngine->m_clock.GetDeltaSquared() * flScale * 60.0f;

            if (bPush)
            {
                m_pAccum->x = (vSide.x + vGap.x) * flScale + m_pAccum->x;
                m_pAccum->y = (vSide.y + vGap.y) * flScale + m_pAccum->y;
                m_pAccum->z = (vSide.z + vGap.z) * flScale + m_pAccum->z;
            }
        }
    }

    char  cWrongSide = 1;
    float flSign     = nSign;

    if (flSign * flDeltaY >= g_flZeroAgendaItem)
    {
        cWrongSide = 0;
    }

    if (m_pDrawTarget != 0 && m_pOwner->GetDistIfFacing(m_pDrawTarget, g_flZeroAgendaItem) != 0.0f)
    {
        float flScale;

        Vec3* const pTargetPos = m_pDrawTarget->GetPositionPtr();
        Vec3* const pOwnPos    = m_pOwner->GetPositionPtr();

        flDeltaX = pTargetPos->x - pOwnPos->x;

        flDeltaY = pTargetPos->y - pOwnPos->y;

        if ((float)(abs((int)flDeltaY) + abs((int)flDeltaX)) < m_flRange)
        {
            vGap.x = flDeltaX;
            vGap.y = flDeltaY;
            vGap.z = pTargetPos->z - pOwnPos->z;

            flRoot = (float)sqrt(vGap.z * vGap.z + flDeltaY * flDeltaY + flDeltaX * flDeltaX);

            flLength = flRoot;

            Math::Vec3Normalize(&vGap);

            if (flLength > g_flHalfAgendaItem)
            {
                flScale = m_flStrength / flLength;
            }
            else
            {
                flScale = g_flTwoAgendaItem * m_flStrength;
            }

            flScale = flScale * g_pGameEngine->m_clock.flDeltaSquared * g_flFrameRateReference;

            if (bPush)
            {
                m_pAccum->x = flScale * vGap.x + m_pAccum->x;
                m_pAccum->y = flScale * vGap.y + m_pAccum->y;
                m_pAccum->z = flScale * vGap.z + m_pAccum->z;
            }
        }
    }

    if (cWrongSide && flSign * flDeltaY < g_flZeroAgendaItem)
    {
        if (g_nWrongSideFrames++ > WRONG_SIDE_LIMIT)
        {
            m_pOwner->PostSceneEvent(MSG_BREAKAWAY, 0);
            g_nWrongSideFrames = 0;
        }
    }

    const Vec3* const pLine  = m_pLine;
    const Vec3* const pAccum = m_pAccum;

    if (pAccum->z * pLine->z + pAccum->y * pLine->y + pAccum->GetX() * pLine->x < 0.3f)
    {
        m_pOwner->PostSceneEvent(MSG_BLOCKED, 0);
    }

    if (m_pNext != 0)
    {
        m_pNext->Process(nArg);
    }

    if (m_cReleaseOnRange != 0 && flLength > m_flRange)
    {
        Detach();
    }
}

// FUNCTION: HYPERX 0x0040fe30
void CAvoidObjects::AddAvoidanceTarget(short nIndex)
{
    if (FindByRef(g_pGameEngine->GetSlot(nIndex)) != 0)
    {
        if (m_pSwerveTarget == 0)
        {
            m_pSwerveTarget = g_pGameEngine->GetSlot(nIndex);
            return;
        }

        if (m_pDrawTarget == 0)
        {
            m_pDrawTarget = g_pGameEngine->GetSlot(nIndex);
            return;
        }

        // STRING: HYPERX 0x004bb744
        g_gameApp.Fatal("Can't avoid more than a coupla things.");
    }
}

// The speed CAvoidObjects needs before it pushes at all, in units a second.
float g_flSwerveMinSpeed = 2.0f;
