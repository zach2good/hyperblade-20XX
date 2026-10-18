// The 32 bytes GameEngine::m_pArena points at; 0x10 counts definitions, 0x14 holds slot ids.

#ifndef HYPERBLADE_ARENASTATE_H
#define HYPERBLADE_ARENASTATE_H

#include <string.h>
#include "physics/collisionellipsoid.h"

class ObjectDef;

struct ArenaState
{
    ArenaState();

    int   m_anUnused[4];
    short m_wCount; // 0x10
    // The slot ids, one int per object. Readers index m_ppSlots with each one.
    int*                      m_pnSlotIds;  // 0x14
    class CollisionEllipsoid* m_pEllipsoid; // 0x18
    // The Rok's definition, which the loader keeps for RokObject::ActivateAtSpawnSlot to respawn it.
    class ObjectDef* m_pRokDef; // 0x1c
};

// Zeroes two members, then clears the whole record over them; only the ellipsoid store survives.
inline ArenaState::ArenaState()
{
    m_wCount    = 0;
    m_pnSlotIds = nullptr;

    memset(this, 0, sizeof(*this));

    m_pEllipsoid = &g_arenaEllipsoid;
}

#endif
