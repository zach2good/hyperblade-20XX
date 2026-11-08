// Small helpers with no subsystem of their own.

#include "compat.h"
#include <stdlib.h>
#include "common/util.h"

// Guards the free, so callers can hand it null.

// FUNCTION: HYPERX100 0x004b8e90
// FUNCTION: HYPERX 0x0044f200
void Util::SafeFree(void* block)
{
    if (block)
    {
        free(block);
    }
}

// FUNCTION: HYPERX100 0x0044f0d0
// FUNCTION: HYPERX 0x00452700
void Util::GetLaserHurdleCharge(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = 1;
}
