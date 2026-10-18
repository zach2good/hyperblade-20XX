// Small helpers with no subsystem of their own.

#include "compat.h"
#include <cstdlib>
#include "common/util.h"

// Guards the free, so callers can hand it null.

// FUNCTION: HYPERX 0x0044f200
void Util::SafeFree(void* block)
{
    if (block)
    {
        free(block);
    }
}

// FUNCTION: HYPERX 0x00452700
void Util::GetLaserHurdleCharge(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = 1;
}
