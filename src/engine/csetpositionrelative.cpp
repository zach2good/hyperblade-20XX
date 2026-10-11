// Mode nought aims at the arena's centre line; mode one parks the marker on the tracked object.

#include "common/compat.h"
#include "engine/messageids.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "devicector.h"
#include "objectclasses.h"
#include "gameengine.h"
#include "planvm/agendaitem.h"
#include "engine/csetpositionrelative.h"
#include "globals.h"
#include "math/vector.h"
#include <math.h>
#include "playerobject.h"
#include "physics/collisionplane.h"
#include "planvm/plannode.h"
#include "common/pooledownerroot.h"

#define MODE_TO_CENTRE 0
#define MODE_ON_OBJECT 1

#define STEP_LENGTH 5.0f
#define DRIFT_LIMIT 25.0

#define MESSAGE_DRIFTED_PARAM 0x96

// FUNCTION: HYPERX 0x0040e490
void CSetPositionRelative::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_pNext     = 0;
    m_wKind     = Agenda_CSetPositionRelative;
    m_bReady    = 0;
    m_pOwner    = pSubject;
    m_pMarker   = 0;
    m_pTracked  = 0;
    m_flUnused  = 120.0f;
    m_vOffset.x = m_vOffset.y = m_vOffset.z = g_flZeroAgendaItem;
    m_wMode                                 = 0;
}

// FUNCTION: HYPERX 0x0040e4e0
void CSetPositionRelative::BindWithNode(EngineObject* pObject,
                                        GameBehaviorObject*,
                                        PlanNode* pNode,
                                        int)
{
    m_wKind  = Agenda_CSetPositionRelative;
    m_pNext  = 0;
    m_bReady = 0;
    m_pOwner = pObject;

    m_pTracked = g_pGameEngine->m_ppSlots[(short)pNode->m_pArgs[1].n];

    m_pMarker  = g_apPositionMarkers[pNode->m_pArgs[2].n];
    m_wMode    = (short)pNode->m_pArgs[3].n;
    m_flUnused = pNode->m_pArgs[4].fl;

    m_vOffset.x = m_vOffset.y = m_vOffset.z = g_flZeroAgendaItem;
}

// FUNCTION: HYPERX 0x0040e560
void CSetPositionRelative::SetPositionWithTerrainHeight(Vec3* pMarker)
{
    m_pMarker = pMarker;

    CollisionShape* const pShell = m_pOwner->GetPhysicsBody()->pShell;

    m_pMarker->z = pShell->GetZ(m_pMarker->x, m_pMarker->y);
}

// FUNCTION: HYPERX 0x0040e5a0
void CSetPositionRelative::SetSourceObjectByIndex(short nIndex)
{
    m_pTracked = g_pGameEngine->m_ppSlots[nIndex];
}

// FUNCTION: HYPERX 0x0040e5c0
void CSetPositionRelative::SetOffset(Vec3 offset)
{
    m_vOffset.x = offset.x;
    m_vOffset.y = offset.y;
    m_vOffset.z = offset.z;
}

// FUNCTION: HYPERX 0x0040e5e0
void CSetPositionRelative::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext     = 0;
        m_pOwner    = 0;
        m_pMarker   = 0;
        m_pTracked  = 0;
        m_vOffset.x = m_vOffset.y = m_vOffset.z = g_flZeroAgendaItem;
        m_wMode                                 = 0;
        m_bReady                                = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x0040e630
void CSetPositionRelative::Process(int nArg)
{
    const Vec3*            pTracked;
    const Vec3*            pOther;
    const TransformObject* pTransform;
    Vec3*                  pMarker;
    float                  flOtherY;
    float                  flZ;
    float                  flY;
    float                  flX;
    Vec3                   vStep;

    switch (m_wMode)
    {
        case MODE_TO_CENTRE:
            pTracked = m_pTracked->GetPositionPtr();

            pOther = g_pGameEngine->m_ppSlots[((PlayerObject*)m_pOwner)->m_wTargetGoalSlot]
                         ->GetPositionPtr();

            flOtherY = pOther->y;

            vStep.x = -pTracked->x;
            vStep.y = flOtherY - pTracked->y;
            vStep.z = g_flZeroAgendaItem;

            Math::Vec3Normalize(&vStep);
            vStep.ScaleInPlace(STEP_LENGTH);

            m_pMarker->x = vStep.x + pTracked->x;
            m_pMarker->y = vStep.y + pTracked->y;

            m_pMarker->z = m_pOwner->GetPhysicsBody()->pShell->GetZ(m_pMarker->x, m_pMarker->y);

            if (fabs(m_pMarker->y - m_pOwner->GetPositionPtr()->y) > DRIFT_LIMIT)
            {
                m_pOwner->PostSceneEvent(MSG_SPEED_BURST, MESSAGE_DRIFTED_PARAM);
            }

            break;

        case MODE_ON_OBJECT:
            flZ        = m_vOffset.z;
            flY        = m_vOffset.y;
            pTransform = &m_pTracked->m_transformC;
            pMarker    = m_pMarker;
            flX        = m_vOffset.x;

            pMarker->x = flX * pTransform->m_a[0][0] + flY * pTransform->m_a[0][1] + flZ * pTransform->m_a[0][2] + pTransform->m_a[0][3];
            pMarker->y = flX * pTransform->m_a[1][0] + flY * pTransform->m_a[1][1] + flZ * pTransform->m_a[1][2] + pTransform->m_a[1][3];
            pMarker->z = flX * pTransform->m_a[2][0] + flY * pTransform->m_a[2][1] + flZ * pTransform->m_a[2][2] + pTransform->m_a[2][3];

            break;
    }

    if (m_pNext != 0)
    {
        m_pNext->Process(nArg);
    }
}
