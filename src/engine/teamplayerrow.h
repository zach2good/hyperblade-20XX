// One player's 0xfc-byte ratings row inside a team block, four to a team from 0x480.

#ifndef HYPERBLADE_TEAMPLAYERROW_H
#define HYPERBLADE_TEAMPLAYERROW_H

#include "statids.h"
#include "common/staticassert.h"

// How many rows a team has. src/engine/teamblock.h's aRows places them.
#define TEAM_ROW_COUNT 4

#define TEAM_ROW_STAT_COUNT 0x3f

struct TeamPlayerRow
{
    float aStat[TEAM_ROW_STAT_COUNT];
};

STATIC_ASSERT(sizeof(TeamPlayerRow) == 0xfc);
#endif
