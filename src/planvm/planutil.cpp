#include "common/compat.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "engine/devicector.h"
#include "planvm.h"
#include "plannode.h"
#include "common/pooledclasses.h"
#include "planvmglobals.h"
#include "collinteractor.h"
#include <cmath>
#include "engine/engineobject.h"
#include "planvm/planutil.h"

#define SCORE_RANGE   250.0f
#define SCORE_FALLOFF 0.004f

// Pi, the plan bank's own copy; marked in src/ui/hudglobals.h.
extern const float g_flPiPlan;

// Three; marked in src/globals.cpp.
extern const float g_flThreePlan;

// An angle, not a distance: the cone half-angle the pass conditions read.
// FUNCTION: HYPERX 0x004600c0
float PlanUtil::ComputeConeHalfAngle(short nSlotA, short nSlotB)
{
    float flFalloff;

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots.get();

    auto* const pObjectA = apObjects[nSlotA];
    auto* const pObjectB = apObjects[nSlotB];

    const Vec3* const pPosA = pObjectA->GetPositionPtr();
    const Vec3* const pPosB = pObjectB->GetPositionPtr();

    const auto flDeltaX = pPosB->x - pPosA->x;
    const auto flDeltaY = pPosB->y - pPosA->y;
    const auto flDeltaZ = pPosB->z - pPosA->z;

    const auto flScale = g_flPiPlan / g_flThreePlan;

    flFalloff = (SCORE_RANGE - static_cast<float>(sqrt(static_cast<double>(flDeltaZ * flDeltaZ + flDeltaY * flDeltaY + flDeltaX * flDeltaX)))) * flScale * SCORE_FALLOFF;

    return flFalloff + flScale;
}

// FUNCTION: HYPERX 0x004693d0
int PlanUtil::CheckSequenceStep(unsigned int nStep)
{
    if (nStep == 0xffffffff)
    {
        return g_nPlanSequenceStep < 12;
    }

    if (g_nPlanSequenceStep < 12)
    {
        if (static_cast<unsigned int>(g_nPlanSequenceStep % 4) == nStep)
        {
            g_nPlanSequenceStep++;

            return 1;
        }
    }

    g_nPlanSequenceStep = 0;

    return 1;
}
