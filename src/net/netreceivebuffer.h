#ifndef HYPERBLADE_NETRECEIVEBUFFER_H
#define HYPERBLADE_NETRECEIVEBUFFER_H

#include "net/directplay.h"
#include "net/netrecords.h"
#include "net/syncblock.h"
#include "common/staticassert.h"

// A system message by type when the sender is DPID_SYSMSG, otherwise a game message by kind.
union NetReceiveBuffer
{
    DPMSG_GENERIC        system;
    DPMSG_ADDPLAYER      addPlayer;
    DPMSG_DELETEPLAYER   deletePlayer;
    NetMessageHeader     message;
    NetPlayerNameMessage playerName;
    SyncBlockHeader      asset;
    MovePacket           move;
    EventsPacket         events;
    StationaryPacket     stationary;
    char                 aBytes[0x100];
};

STATIC_ASSERT(sizeof(NetReceiveBuffer) == 0x100);

#endif
