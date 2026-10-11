#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "planvm/agendaitemids.h"
#include "common/behobjglidetoposition.h"
#include "planvm/agendaitem.h"
#include "planvm/crelaxtoposition.h"
#include "physics/collisionellipsoid.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "planvm.h"
#include "plannode.h"
#include "engine/objectclasses.h"
#include "physics/physicsbody.h"
#include "engine/objectstats.h"
#include "math/vector.h"
#include "common/stub.h"
#include "engine/devicector.h"
#include <math.h>
#include "engine/playerobject.h"
#include "engine/engineobject.h"
#include "engine/cameraobject.h"
#include "common/pooledownerroot.h"
#include "engine/statids.h"

// Four arguments copy a marker; five put an x and y on the floor; any other count zeroes it.

// The marker node names a marker and a slot; the point node names x, y and a slot.
#define ARG_FLAGS       1
#define ARG_MARKER      2
#define ARG_MARKER_SLOT 3
#define ARG_X           2
#define ARG_Y           3
#define ARG_POINT_SLOT  4

#define ARG_COUNT_MARKER 4
#define ARG_COUNT_POINT  5

// FUNCTION: HYPERX 0x00411c90
void CRelaxToPosition::Bind(EngineObject* pSubject, GameBehaviorObject* pNode)
{
    m_pNext       = 0;
    m_wKind       = Agenda_CRelaxToPosition;
    m_pOwner      = pSubject;
    m_bReady      = 0;
    m_bCameraBias = 0;
    m_pTarget     = 0;

    if (pNode->m_wKind == BEH_GLIDE_TO_POSITION)
    {
        m_vDest = ((BehObj_GlideToPosition*)pNode)->m_vTarget;
        return;
    }

    m_vDest.x = m_vDest.y = m_vDest.z = g_flZeroAgendaItem;
}

// FUNCTION: HYPERX 0x00411cf0
void CRelaxToPosition::BindWithNode(EngineObject* pObject, GameBehaviorObject*, PlanNode* pNode, int)
{
    PlanWord* pArgs;

    m_wKind       = Agenda_CRelaxToPosition;
    m_pNext       = 0;
    m_bReady      = 0;
    m_bCameraBias = 0;
    m_pOwner      = pObject;

    m_bCameraBias = (char)pNode->m_pArgs[ARG_FLAGS].n;

    if (pNode->m_nArgCount == ARG_COUNT_MARKER)
    {
        pArgs = pNode->m_pArgs;

        m_vDest = *g_apPositionMarkers[pArgs[ARG_MARKER].n];

        pArgs = pNode->m_pArgs;

        if (pArgs[ARG_MARKER_SLOT].IsSelector())
        {
            m_pTarget =
                g_pGameEngine->m_ppSlots
                    [g_apPlanStatFns[pArgs[ARG_MARKER_SLOT].wSelector](pObject, pNode)];

            return;
        }

        m_pTarget = g_pGameEngine->m_ppSlots[(short)pArgs[ARG_MARKER_SLOT].n];

        return;
    }

    if (pNode->m_nArgCount == ARG_COUNT_POINT)
    {
        m_vDest.x = (float)pNode->m_pArgs[ARG_X].n;

        m_vDest.y = (float)pNode->m_pArgs[ARG_Y].n;

        m_vDest.z = g_arenaEllipsoid.GetZ(m_vDest.x, m_vDest.y);

        pArgs = pNode->m_pArgs;

        if (pArgs[ARG_POINT_SLOT].IsSelector())
        {
            m_pTarget =
                g_pGameEngine->m_ppSlots
                    [g_apPlanStatFns[pArgs[ARG_POINT_SLOT].wSelector](pObject, pNode)];

            return;
        }

        m_pTarget = g_pGameEngine->m_ppSlots[(short)pArgs[ARG_POINT_SLOT].n];

        return;
    }

    m_vDest.x = m_vDest.y = m_vDest.z = g_flZeroAgendaItem;
    m_pTarget                         = 0;
}

// Eases toward the destination at a rate that does not depend on the frame rate.

// Fifty two is the last frame rate that reads a slower rate than the one before it.
#define FRAME_RATE_LIMIT 0x34

// Sixteen either way.
#define BIAS_SCALE 16.0f

// A fifth, at 0x004b1658.
#define RATE_TRIM 0.2f

// The rate a frame closes, by frame rate over four; the clamp leaves the last entry unreachable.
// GLOBAL: HYPERX 0x004bb674
extern float g_aRelaxRateByFrameRate[15];

// FUNCTION: HYPERX 0x00411e30
void CRelaxToPosition::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext  = 0;
        m_pOwner = 0;
        m_bReady = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x00411e60
void CRelaxToPosition::Process(int nArg)
{
    Vec3* pNormal;
    Vec3  vGap;
    Vec3  vStep;
    float flSign;
    int   nClamped;

    Vec3* const pPoint = &m_pOwner->GetPhysicsBody()->pos;

    Vec3 vTarget = m_vDest;

    const short nBucket = (short)g_pGameEngine->m_clock.flInvDelta;

    nClamped = min(FRAME_RATE_LIMIT, nBucket);

    nClamped >>= 2;

    float flRate = g_aRelaxRateByFrameRate[(short)nClamped];

    if (m_bCameraBias != 0)
    {
        if (m_vDest.y < 0.0f)
        {
            flSign = -BIAS_SCALE;
        }
        else
        {
            flSign = BIAS_SCALE;
        }

        vTarget.x = (g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT)->m_flViewFracX - g_flHalfAgendaItem) * flSign + vTarget.x;

        flRate = ((PlayerObject*)m_pOwner)->m_stats.GetScalar(RATING_CHECKING) * flRate * RATE_TRIM;
    }

    vGap.x = vTarget.x - pPoint->x;
    vGap.y = vTarget.y - pPoint->y;
    vGap.z = vTarget.z - pPoint->z;

    Vec3::ScaleTo(&vGap, flRate, &vStep);

    const float flLength = vStep.Magnitude();

    if ((flRate = m_pOwner->GetPhysicsBody()->flForwardSpeedDt) < flLength)
    {
        Math::Vec3Normalize(&vStep);
        vStep.ScaleInPlace(flRate);
    }

    pPoint->x = vStep.x + pPoint->x;
    pPoint->y = vStep.y + pPoint->y;
    pPoint->z = vStep.z + pPoint->z;

    if (m_pTarget != 0)
    {
        Vec3* const pHeading = &m_pOwner->GetPhysicsBody()->velDir;

        pNormal = m_pTarget->GetPositionPtr();

        pHeading->x = pNormal->x - pPoint->x;
        pHeading->y = pNormal->y - pPoint->y;
        pHeading->z = pNormal->z - pPoint->z;

        Math::Vec3Normalize(pHeading);
    }

    PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

    pNormal = g_arenaEllipsoid.NormalAtPoint(&m_pOwner->GetPhysicsBody()->pos);

    pBody->surfNormal = *pNormal;

    if (m_pNext != 0)
    {
        m_pNext->Process(nArg);
    }
}

// The same routine on the staged point at 0xf4, run in phase three, then published.

// FUNCTION: HYPERX 0x00412050
void CRelaxToPosition::Release(int nArg)
{
    Vec3* pNormal;
    Vec3  vGap;
    Vec3  vStep;
    float flSign;
    int   nClamped;

    Vec3* const pPoint = &m_pOwner->GetPhysicsBody()->netTarget;

    Vec3 vTarget = m_vDest;

    const short nBucket = (short)g_pGameEngine->m_clock.flInvDelta;

    nClamped = min(FRAME_RATE_LIMIT, nBucket);

    nClamped >>= 2;

    float flRate = g_aRelaxRateByFrameRate[(short)nClamped];

    if (m_bCameraBias != 0)
    {
        if (m_vDest.y < 0.0f)
        {
            flSign = -BIAS_SCALE;
        }
        else
        {
            flSign = BIAS_SCALE;
        }

        vTarget.x = (g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT)->m_flViewFracX - g_flHalfAgendaItem) * flSign + vTarget.x;

        flRate = ((PlayerObject*)m_pOwner)->m_stats.GetScalar(RATING_CHECKING) * flRate * RATE_TRIM;
    }

    vGap.x = vTarget.x - pPoint->x;
    vGap.y = vTarget.y - pPoint->y;
    vGap.z = vTarget.z - pPoint->z;

    Vec3::ScaleTo(&vGap, flRate, &vStep);

    const float flLength = vStep.Magnitude();

    if ((flRate = m_pOwner->GetPhysicsBody()->flForwardSpeedDt) < flLength)
    {
        Math::Vec3Normalize(&vStep);
        vStep.ScaleInPlace(flRate);
    }

    pPoint->x = vStep.x + pPoint->x;
    pPoint->y = vStep.y + pPoint->y;
    pPoint->z = vStep.z + pPoint->z;

    if (m_pTarget != 0)
    {
        Vec3* const pHeading = &m_pOwner->GetPhysicsBody()->velDir;

        pNormal = m_pTarget->GetPositionPtr();

        pHeading->x = pNormal->x - pPoint->x;
        pHeading->y = pNormal->y - pPoint->y;
        pHeading->z = pNormal->z - pPoint->z;

        Math::Vec3Normalize(pHeading);
    }

    PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();

    pNormal = g_arenaEllipsoid.NormalAtPoint(pPoint);

    pBody->surfNormal = *pNormal;

    m_pOwner->GetPhysicsBody()->pos = *pPoint;

    if (m_pNext != 0)
    {
        m_pNext->Release(nArg);
    }
}

// Indexed by frame rate, clamped at 52, over four; the last entry is unreachable.
float g_aRelaxRateByFrameRate[15] = { 0.75f, 0.29f, 0.159f, 0.1091f, 0.0829f, 0.067f, 0.056f, 0.0483f, 0.0424f, 0.0377f, 0.035f, 0.031f, 0.0285f, 0.0263f, 0.024f };
