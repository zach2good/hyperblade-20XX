// The 32 bytes GameEngine::m_pArena points at; 0x10 counts definitions, 0x14 holds slot ids.

#ifndef HYPERBLADE_ARENASTATE_H
#define HYPERBLADE_ARENASTATE_H

#include "engine/objectdef.h"
#include "physics/collisionellipsoid.h"
#include <memory>

struct ArenaState
{
    ArenaState();

    int   m_anUnused[4] = {};
    short m_wCount      = 0; // 0x10
    // The slot ids, one int per object. Readers index m_ppSlots with each one.
    std::unique_ptr<int[]>    m_pnSlotIds;  // 0x14
    class CollisionEllipsoid* m_pEllipsoid; // 0x18
    // The Rok's definition, which the loader keeps for RokObject::ActivateAtSpawnSlot to respawn it.
    std::unique_ptr<ObjectDef> m_pRokDef; // 0x1c
};

inline ArenaState::ArenaState()
{
    m_pEllipsoid = &g_arenaEllipsoid;
}

#endif
