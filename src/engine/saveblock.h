// The saved game record: a label, two team runs of 0x32c bytes, and two bytes of padding at 0x6ba.

#ifndef HYPERBLADE_SAVEBLOCK_H
#define HYPERBLADE_SAVEBLOCK_H

#include "teamblock.h"

constexpr auto kSaveBlockTagValue = 0x314d444d;

struct SaveTeam
{
    int         nTeamIndex;               // 0x000
    TeamStatRow aStatRows[kTeamRowCount]; // 0x004
    float       aDamage[kTeamRowCount];   // 0x314, one per row, moved as an int
    int         aSeat[kTeamBlockSeats];   // 0x324
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
