// The game's wall clock.

#ifndef HYPERBLADE_GAMETIME_H
#define HYPERBLADE_GAMETIME_H

// What GameTime::Now fills: three words, the layout of the runtime's _timeb.
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
