// The network session and its players; the command pump skips entries zero and fifteen.

#ifndef HYPERBLADE_SESSION_H
#define HYPERBLADE_SESSION_H

#include "engine/devicector.h"
#include "net/playerslotname.h"

struct PlayerState
{
    unsigned int nPlayerId; // 0x00, unknown until the player is registered
    // Set once DirectPlay registers the player.
    int nConnected; // 0x04
    // The name byte's human bit.
    int nHuman; // 0x08
    // The engine slot the player drives, MODEL_ID_NONE for none.
    int            nObjectSlot;   // 0x0c
    PlayerSlotName slotName;      // 0x10
    int            nSlotFlags;    // 0x50
    int            bHost;         // 0x54
    int            nReady;        // 0x58
    int            nActive;       // 0x5c
    int            bLocal;        // 0x60
    int            nDisconnected; // 0x64
};

// The local player: the teardown destroys it by name, and the command pump skips it.
constexpr auto SESSION_LOCAL_PLAYER = 15;

constexpr auto SESSION_PLAYER_SLOTS = 16;
constexpr auto SESSION_REMOTE_IDS   = 8;

// The network slots that drive the Rok and the referee, God.
constexpr auto NET_SLOT_ROK = 7;
constexpr auto NET_SLOT_GOD = 8;

#endif
