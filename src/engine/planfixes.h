#ifndef HYPERBLADE_PLANFIXES_H
#define HYPERBLADE_PLANFIXES_H

#include <optional>

// The fixes for the plan bugs in docs/plan_bugs.md: the fixed plans every object compiles beside
// the shipped ones, and the engine faults those plans run into.
namespace PlanFixes
{

bool On();

// Moves every live object onto the other compiled plans, in the behaviour it was in. A network
// session keeps whatever it started with.
void Set(bool bOn);

} // namespace PlanFixes

// What a green power-up puts back when it ends, with the fixes on. Empty while none runs.
struct PowerUpRestore
{
    std::optional<float> oflSpeed;
    std::optional<float> oflLongShot;
    std::optional<float> oflLongPass;
    std::optional<float> oflImmunity;
};

#endif
