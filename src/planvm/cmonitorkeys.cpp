// Carries the input word from KeyboardDeviceObj::m_nHeldMask on object 0x56 into the player and back.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "engine/objectclasses.h"
#include "engine/gameengine.h"
#include "engine/devicector.h"
#include "physics/physicsbody.h"
#include "planvm/agendaitem.h"
#include "planvm/cmonitorkeys.h"
#include "globals.h"
#include "engine/engineobject.h"
#include "input/keyboarddeviceobj.h"
#include "physics/collisionellipsoid.h"
#include "engine/player.h"
#include "common/pooledownerroot.h"

#define ARENA_LIMIT 6.0
#define CLAMP_RATE  5.0f

// Four, the arena edge margin; marked in src/globals.cpp.
extern const float g_flFourPlayer;

// FUNCTION: HYPERX 0x00410df0
void CMonitorKeys::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = 0;
    m_wKind  = Agenda_CMonitorKeys;
    m_bReady = 0;
    m_pOwner = pOwner;

    short* pFirst = &pOwner->GetPhysicsBody()->nHeldBindings;
    m_pInput      = pFirst;
    *pFirst       = (short)g_pGameEngine->GetInputDevice(SLOT_KEYBOARD)->m_nHeldMask;

    short* pSecond = &pOwner->GetPhysicsBody()->nPrevHeldBindings;
    m_pPrevInput   = pSecond;
    *pSecond       = 0;
    m_nMode        = MON_DRIVE_YER_GUY;
}

// FUNCTION: HYPERX 0x00410e60
void CMonitorKeys::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext      = 0;
        m_pOwner     = 0;
        m_pInput     = 0;
        m_pPrevInput = 0;
        m_nMode      = MON_DRIVE_YER_GUY;
        m_bReady     = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x00410e90
void CMonitorKeys::Process(int nArg)
{
    float        flEdge;
    PhysicsBody* pBlock;

    *m_pInput = (short)g_pGameEngine->GetInputDevice(SLOT_KEYBOARD)->m_nHeldMask;

    Player::ProcessTrickInput(m_pOwner, m_pInput);

    switch (m_nMode)
    {
        case MON_GOALIE_CTRL:
            pBlock = m_pOwner->GetPhysicsBody();

            m_pOwner->GetPhysicsBody()->velDirLatched = pBlock->velDir;

            if (m_pOwner->GetPositionPtr()->x < -ARENA_LIMIT && m_pOwner->GetPhysicsBody()->axis.y > g_flZeroAgendaItem)
            {
                m_pOwner->GetPhysicsBody()->axis.y = g_flZeroAgendaItem;
            }

            if (m_pOwner->GetPositionPtr()->x > g_dHomingSix && m_pOwner->GetPhysicsBody()->axis.y < g_flZeroAgendaItem)
            {
                m_pOwner->GetPhysicsBody()->axis.y = g_flZeroAgendaItem;
            }

            flEdge = g_arenaEllipsoid.GetRadiusY();

            if (flEdge - m_pOwner->GetPositionPtr()->y > g_flFourPlayer && m_pOwner->GetPhysicsBody()->axis.x > g_flZeroAgendaItem)
            {
                m_pOwner->GetPhysicsBody()->axis.x = g_flZeroAgendaItem;
            }

            flEdge = g_arenaEllipsoid.GetRadiusY();

            if (flEdge - m_pOwner->GetPositionPtr()->y < -g_flFourPlayer && m_pOwner->GetPhysicsBody()->axis.x < g_flZeroAgendaItem)
            {
                m_pOwner->GetPhysicsBody()->axis.x = g_flZeroAgendaItem;
            }

            m_pOwner->GetPositionPtr()->x = m_pOwner->GetPhysicsBody()->pos.x -= m_pOwner->GetPhysicsBody()->axis.y * CLAMP_RATE;

            m_pOwner->GetPositionPtr()->y = m_pOwner->GetPhysicsBody()->pos.y -= m_pOwner->GetPhysicsBody()->axis.x * CLAMP_RATE;
            break;

        case MON_DRIVE_YER_GUY:
        case MON_ARCADE_STYLE:
            m_pOwner->GetPhysicsBody()->ApplySteeringForce();
            break;
    }

    *m_pPrevInput = *m_pInput;

    g_pGameEngine->GetInputDevice(SLOT_KEYBOARD)->m_nHeldMask = (unsigned short)*m_pPrevInput;

    AgendaItem* const pNext = m_pNext;

    if (pNext != 0)
    {
        pNext->Process(nArg);
    }
}
