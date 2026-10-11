// The compile-time selector bank, right after the runtime bank g_apPlanStatFns.

#ifndef HYPERBLADE_PLANOPCODETABLE_H
#define HYPERBLADE_PLANOPCODETABLE_H

#include "engine/devicector.h"
#include "planarg.h"

class EngineObject;
union PlanWord;

typedef short(__cdecl* PlanArgFn)(EngineObject** ppActor, const PlanWord* pStream, int* pResult);

// GLOBAL: HYPERX 0x004d187c
extern PlanArgFn g_apPlanArgFns[PRN_COUNT];

#endif
