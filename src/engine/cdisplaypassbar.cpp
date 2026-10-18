// Points the HUD's pass bar at the team mate of whoever carries the rok.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "gameengine.h"
#include "physics/physicsbody.h"
#include "devicector.h"
#include "objectclasses.h"
#include "planvm/agendaitem.h"
#include "engine/cdisplaypassbar.h"
#include "globals.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "math/vector.h"
#include "physics/banking.h"
#include "engine/engineobject.h"
#include "graphics/viewextent.h"

// 1 / (1 - cos 45), so the offset is the whole half width at 45 degrees off the heading.
#define OVERLAY_EDGE_SCALE 3.4142139f

// FUNCTION: HYPERX 0x00412530
void CDisplayPassBar::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind  = Agenda_CDisplayPassBar;
    m_pNext  = nullptr;
    m_bReady = 0;
    m_pOwner = pSubject;
}

// FUNCTION: HYPERX 0x00412550
void CDisplayPassBar::Process(int nArg)
{
    PhysicsBody* const pRokBody = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody();
    AttachPak* const   pCarrier = pRokBody->pAttach;

    if (pCarrier != nullptr)
    {
        short nSlot = pCarrier->nSlot;

        if (nSlot < PLAYER_OBJECT_COUNT && nSlot == g_pGameEngine->m_nViewedSlot && g_pRenderer != nullptr && m_pOwner->GetSlot() == CAMERA_MAIN_SLOT)
        {
            const int     nSelf = nSlot;
            EngineObject* pSelf = g_pGameEngine->m_ppSlots[nSelf];
            Vec3          vSelf;
            vSelf = *pSelf->GetPositionPtr();

            Vec3* const pMatePos = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)]->GetPositionPtr();

            ViewExtent pos;

            pos = g_pRenderer->GetPassBarSize();

            const float flHalf = (float)(pos.y >> 1);

            Vec3 vDir;
            vDir.x = pMatePos->x - vSelf.x;
            vDir.y = pMatePos->y - vSelf.y;
            vDir.z = pMatePos->z - vSelf.z;

            Math::Vec3Normalize(&vDir);

            const Vec3* pHeading = &pSelf->GetPhysicsBody()->velDir;

            const float flDot = vDir.x * pHeading->x + vDir.y * pHeading->y + vDir.z * pHeading->z;

            const Vec3* pSide = &pSelf->GetPhysicsBody()->velDir;

            float flOffset;
            int   nPos;

            if (vDir.x * pSide->y - vDir.y * pSide->x > 0.0)
            {
                flOffset = (g_flOneAgendaItem - flDot) * OVERLAY_EDGE_SCALE;
                flOffset *= flHalf;
                nPos = (int)(flOffset + flHalf);
            }
            else
            {
                flOffset = (g_flOneAgendaItem - flDot) * OVERLAY_EDGE_SCALE;
                flOffset *= flHalf;
                nPos = (int)(flHalf - flOffset);
            }

            g_pRenderer->SetPassBarMarker(nPos);

            g_pRenderer->ShowPassBar();
        }
    }

    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr && m_bReady == 0)
    {
        pNext->Process(nArg);
    }
}
