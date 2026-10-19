// Small helpers with no subsystem of their own.

#include "compat.h"
#include "common/util.h"

// FUNCTION: HYPERX 0x00452700
void Util::GetLaserHurdleCharge(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = 1;
}
