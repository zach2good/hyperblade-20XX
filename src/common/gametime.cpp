#include "common/compat.h"
#include "common/gametime.h"

#include <chrono>

// FUNCTION: HYPERX 0x0048fc40
void GameTime::Now(SystemTimePair* pTime)
{
    const auto nMillis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    pTime->nSeconds         = static_cast<int>(nMillis / 1000);
    pTime->nMillis          = static_cast<int>(nMillis % 1000);
    pTime->nZoneAndDaylight = 0;
}
