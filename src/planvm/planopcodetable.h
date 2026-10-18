// The compile-time pronoun resolver bank, right after the runtime bank g_apPlanStatFns.

#ifndef HYPERBLADE_PLANOPCODETABLE_H
#define HYPERBLADE_PLANOPCODETABLE_H

#include "engine/devicector.h"
#include "planarg.h"

class EngineObject;
union PlanWord;

using PlanArgFn = short (*)(EngineObject** ppActor, const PlanWord* pStream, int* pResult);

// GLOBAL: HYPERX 0x004d187c
extern PlanArgFn g_apPlanArgFns[PRN_COUNT];

#endif
