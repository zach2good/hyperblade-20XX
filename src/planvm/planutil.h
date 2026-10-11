#ifndef HYPERBLADE_PLANUTIL_H
#define HYPERBLADE_PLANUTIL_H

#include "common/compat.h"

namespace PlanUtil
{

float __cdecl ComputeConeHalfAngle(short nSlotA, short nSlotB);
int __cdecl   CheckSequenceStep(unsigned int nStep);

} // namespace PlanUtil

#endif
