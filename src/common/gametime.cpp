#include "common/compat.h"
#include "common/gametime.h"

#include <sys/timeb.h>

// FUNCTION: HYPERX 0x0048fc40
void GameTime::Now(SystemTimePair* pTime)
{
    static_assert(sizeof(__timeb32) == sizeof(SystemTimePair));

    _ftime32((__timeb32*)pTime);
}
