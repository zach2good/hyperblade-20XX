#ifndef HYPERBLADE_UTIL_H
#define HYPERBLADE_UTIL_H

#include "common/compat.h"

class Interactor;
struct PlanNode;

namespace Util
{

void GetLaserHurdleCharge(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SafeFree(void* block);

} // namespace Util

#endif
