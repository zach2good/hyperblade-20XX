// The difficulty's damage step, shared by plan code, physics and the HUD.

#ifndef HYPERBLADE_PLANVM_DAMAGEGATE_H
#define HYPERBLADE_PLANVM_DAMAGEGATE_H

#include "engine/devicector.h"
#include "engine/gameengine.h"

enum DifficultyLevel
{
    DIFFICULTY_EASY   = 0,
    DIFFICULTY_MEDIUM = 1,
    DIFFICULTY_HARD   = 2,

    // The last level, which src/engine/gameengine.cpp clamps against.
    DIFFICULTY_MAX = 2
};

// The third arm's compare is dead: nine is taken whatever the level.
inline float DamageGate()
{
    if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_EASY)
    {
        return 3.0f;
    }

    if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_MEDIUM)
    {
        return 6.0f;
    }

    if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_HARD)
    {
        return 9.0f;
    }

    return 9.0f;
}

#endif
