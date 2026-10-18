// One player's 0xfc-byte ratings row inside a team block, four to a team from 0x480.

#ifndef HYPERBLADE_TEAMPLAYERROW_H
#define HYPERBLADE_TEAMPLAYERROW_H

#include "statids.h"

// How many rows a team has. src/engine/teamblock.h's aRows places them.
constexpr auto kTeamRowCount = 4;

constexpr auto kTeamRowStatCount = 0x3f;

struct TeamPlayerRow
{
    float aStat[kTeamRowStatCount];
};

static_assert(sizeof(TeamPlayerRow) == 0xfc);
#endif
