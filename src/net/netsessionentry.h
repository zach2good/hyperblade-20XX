// One enumerated session: DirectPlay's description copied whole, with the two user words the
// application owns holding the list link and the live flag.

#ifndef HYPERBLADE_NETSESSIONENTRY_H
#define HYPERBLADE_NETSESSIONENTRY_H
#include "common/compat.h"

#include "common/staticassert.h"
#include "netglobals.h"
#include "net/directplay.h"
#include <stddef.h>

struct NetSessionEntry
{
    DWORD            dwSize;
    GUID             guidSession;
    DWORD            dwSession;
    DWORD            dwMaxPlayers;
    DWORD            dwCurrentPlayers;
    DWORD            dwFlags;
    char             szSessionName[DPSESSIONNAMELEN];
    char             szUserField[DPUSERRESERVED];
    DWORD            dwReserved1;
    char             szPassword[DPPASSWORDLEN];
    DWORD            dwReserved2;
    NetSessionEntry* m_pNext; // dwUser1
    int              m_bLive; // dwUser2
    DWORD            dwUser3;
    DWORD            dwUser4;
};

STATIC_ASSERT(sizeof(NetSessionEntry) == sizeof(DPSESSIONDESC));
STATIC_ASSERT(offsetof(NetSessionEntry, m_pNext) == offsetof(DPSESSIONDESC, dwUser1));
#endif
