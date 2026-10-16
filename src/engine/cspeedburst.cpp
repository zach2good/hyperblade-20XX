// Stamina decays 1.5 times faster while the player carries the rok, floored at a twentieth.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "gameengine.h"
#include "physics/physicsbody.h"
#include "objectclasses.h"
#include "devicector.h"
#include "planvm/agendaitem.h"
#include "engine/cspeedburst.h"
#include "engine/objectstats.h"
#include "globals.h"
#include <new.h>
#include "playerobject.h"
#include "engine/engineobject.h"
#include "common/pooledownerroot.h"

#define CARRIER_RATE  1.5f
#define DECAY_RATE    0.8f
#define STAMINA_FLOOR 0.05f
#define BOOST_EPSILON 1e-5f

// FUNCTION: HYPERX 0x004112e0
CSpeedBurst::CSpeedBurst() throw()
{
    m_pOpponentA = 0;
    m_pOpponentB = 0;
    m_pBoost     = 0;
    m_flTarget   = g_flZeroAgendaItem;
}

// FUNCTION: HYPERX 0x00411310
void CSpeedBurst::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = 0;
    m_wKind  = Agenda_CSpeedBurst;
    m_bReady = 0;
    m_pOwner = pOwner;

    // The player slots either side of the owner's, the opponents, wrapping through four.
    int nOwnerSlot = pOwner->GetSlot();

    m_pOpponentA = g_pGameEngine->m_ppSlots[(short)((nOwnerSlot + 1) % 4)];

    nOwnerSlot = m_pOwner->GetSlot();

    m_pOpponentB = g_pGameEngine->m_ppSlots[(short)((nOwnerSlot + 3) % 4)];

    m_pBoost   = &pOwner->GetPhysicsBody()->flSteerScale;
    m_flTarget = g_flZeroAgendaItem;
}

// FUNCTION: HYPERX 0x004113a0
void CSpeedBurst::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext      = 0;
        m_pOwner     = 0;
        m_pOpponentA = 0;
        m_pOpponentB = 0;

        *m_pBoost = g_flOneAgendaItem;

        m_pBoost   = 0;
        m_bReady   = 1;
        m_flTarget = g_flZeroAgendaItem;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x004113f0
void CSpeedBurst::Process(int nArg)
{
    float      flDelta;
    float      flRate;
    AttachPak* pCarrier;
    short      nCarrier;

    if (m_bReady != 0)
    {
        Detach();
        return;
    }

    if (*m_pBoost > g_flOneAgendaItem)
    {
        bool  bFloored  = false;
        float flStamina = ((PlayerObject*)m_pOwner)->m_stats.m_flSpeedRatio;
        flDelta         = g_pGameEngine->m_clock.flDelta;

        pCarrier = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        if (pCarrier != 0 && (nCarrier = pCarrier->nSlot) < PLAYER_OBJECT_COUNT && nCarrier == m_pOwner->GetSlot())
        {
            flRate = flDelta * CARRIER_RATE;
        }
        else
        {
            flRate = flDelta * g_flOneAgendaItem;
        }

        flStamina = (g_flOneAgendaItem - flRate) * flStamina;

        if (flStamina < STAMINA_FLOOR)
        {
            flStamina = STAMINA_FLOOR;
            bFloored  = true;
        }

        ((PlayerObject*)m_pOwner)->m_stats.m_flSpeedRatio = flStamina;

        if (m_flTarget > *m_pBoost)
        {
            *m_pBoost = g_pGameEngine->m_clock.flDelta + *m_pBoost;
        }
        else if (bFloored)
        {
            const float flDecay = g_flOneAgendaItem - flDelta * DECAY_RATE;
            m_flTarget          = g_flZeroAgendaItem;
            *m_pBoost           = flDecay * *m_pBoost;

            if (GetBoost() < g_flOneAgendaItem + BOOST_EPSILON)
            {
                *m_pBoost = g_flOneAgendaItem;

                if (m_pNext != 0)
                {
                    m_pNext->Process(nArg);
                }

                Detach();
                return;
            }
        }
    }

    if (m_pNext != 0 && m_bReady == 0)
    {
        m_pNext->Process(nArg);
    }
}
