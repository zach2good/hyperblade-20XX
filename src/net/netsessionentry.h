// One enumerated session: DirectPlay's description copied whole, then the list link and the live
// flag.

#ifndef HYPERBLADE_NETSESSIONENTRY_H
#define HYPERBLADE_NETSESSIONENTRY_H
#include "common/compat.h"

#include "netglobals.h"
#include "net/directplay.h"

// The link and the live flag follow DirectPlay's description, so a pointer never has to fit a
// user word.
struct NetSessionEntry : DPSESSIONDESC
{
    NetSessionEntry* m_pNext;
    int              m_bLive;
};

#endif
