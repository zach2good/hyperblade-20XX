#ifndef HYPERBLADE_PLANUTIL_H
#define HYPERBLADE_PLANUTIL_H

#include "common/compat.h"

namespace PlanUtil
{

float ComputeConeHalfAngle(short nSlotA, short nSlotB);
int   CheckSequenceStep(unsigned int nStep);

} // namespace PlanUtil

#endif
