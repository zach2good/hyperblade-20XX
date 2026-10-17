// The game's wall clock.

#ifndef HYPERBLADE_GAMETIME_H
#define HYPERBLADE_GAMETIME_H

// What GameTime::Now fills: three words, the layout of the runtime's 32-bit __timeb32, with no zone.
struct SystemTimePair
{
    int nSeconds;
    int nMillis;
    int nZoneAndDaylight;
};

namespace GameTime
{

void Now(SystemTimePair* pTime);

}

#endif
