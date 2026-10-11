// The two tables that name a network slot, and what each one drives.

#include "common/compat.h"
#include "globals.h"
#include "session.h"
#include <string.h>
#include "cplayer.h"
#include "gamesession.h"
#include "app/gameapp.h"

// Which engine object each slot drives; 102 drives nothing.
// GLOBAL: HYPERX 0x004c0dc8
int g_anNetworkSlotObject[] = {
    102, 1, 0, 3, 2, 5, 4, 6, 23, 54, 55, 102, 102, 102, 102, 0
};

// What to call each slot when refusing it.
// GLOBAL: HYPERX 0x004c0e08
const char* g_apNetworkSlotNames[] = {
    "Reserved      ", "Away Striker", "Home Striker", "Away Defender", "Home Defender", "Away Goalie", "Home Goalie", "Rok", "Ref (God)", "Mine", "Swarf", "Reserved B", "Reserved C", "Reserved D", "Reserved E", "Packet Carrier"
};
