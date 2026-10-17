// A networked player's link to the object it drives. Networking is stubbed out, so no physics body
// has one.

#ifndef HYPERBLADE_CPLAYER_H
#define HYPERBLADE_CPLAYER_H

#include "net/netrecords.h"
#include "net/session.h"
#include <mutex>

class CPlayer
{
public:
    // The next move received for the object, or null.
    NetMoveRecord* DequeueNetRecord();

    // Guards the object state the network side applies.
    std::recursive_mutex m_lock;
    PlayerState*         pState = nullptr;
};

#endif
