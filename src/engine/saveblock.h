// The saved game record: a label, two team runs of 0x32c bytes, and two bytes of padding at 0x6ba.

#ifndef HYPERBLADE_SAVEBLOCK_H
#define HYPERBLADE_SAVEBLOCK_H

#include "teamblock.h"

constexpr auto SAVE_BLOCK_TAG_VALUE = 0x314d444d;

struct SaveTeam
{
    int         nTeamIndex;                // 0x000
    TeamStatRow aStatRows[TEAM_ROW_COUNT]; // 0x004
    float       aDamage[TEAM_ROW_COUNT];   // 0x314, one per row, moved as an int
    int         aSeat[TEAM_BLOCK_SEATS];   // 0x324
};

static_assert(sizeof(SaveTeam) == 0x32c);

struct SaveBlock
{
    int      nTag;          // 0x000, 'MDM1'
    char     szLabel[0x50]; // 0x004
    int      nBracketPos;   // 0x054
    int      nDifficulty;   // 0x058
    SaveTeam teamOne;       // 0x05c
    SaveTeam teamTwo;       // 0x388
    char     abAward[6];    // 0x6b4, GameEngine::m_abAward
};

static_assert(sizeof(SaveBlock) == 0x6bc);
#endif
