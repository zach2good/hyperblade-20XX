#include "common/compat.h"
#include "common/gametime.h"

#include <sys/timeb.h>

// FUNCTION: HYPERX 0x0048fc40
void GameTime::Now(SystemTimePair* pTime)
{
    _ftime((struct _timeb*)pTime);
}
