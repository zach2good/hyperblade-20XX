// The difficulty's damage step, shared by plan code, physics and the HUD.

#ifndef HYPERBLADE_PLANVM_DAMAGEGATE_H
#define HYPERBLADE_PLANVM_DAMAGEGATE_H

#include "engine/devicector.h"
#include "engine/gameengine.h"

enum class DifficultyLevel
{
    kEasy   = 0,
    kMedium = 1,
    kHard   = 2,

    // The last level, which src/engine/gameengine.cpp clamps against.
    kMax = 2
};

// The third arm's compare is dead: nine is taken whatever the level.
inline float DamageGate()
{
    if (g_pGameEngine->GetDifficultyLevel() == DifficultyLevel::kEasy)
    {
        return 3.0f;
    }

    if (g_pGameEngine->GetDifficultyLevel() == DifficultyLevel::kMedium)
    {
        return 6.0f;
    }

    if (g_pGameEngine->GetDifficultyLevel() == DifficultyLevel::kHard)
    {
        return 9.0f;
    }

    return 9.0f;
}

#endif
