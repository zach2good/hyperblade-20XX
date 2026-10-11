#ifndef HYPERBLADE_NETSENDBUFFER_H
#define HYPERBLADE_NETSENDBUFFER_H

#include "net/netrecords.h"
#include "common/staticassert.h"

// A game message by kind; the kind byte opens every member.
union NetSendBuffer
{
    NetMessageHeader message;
    NetDataMessage   data;
    MovePacket       move;
    EventsPacket     events;
    StationaryPacket stationary;
    unsigned char    aBytes[0x100];
};

STATIC_ASSERT(sizeof(NetSendBuffer) == 0x100);

#endif
