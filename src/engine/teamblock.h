// The 0x888-byte team block both team pointers name.

#ifndef HYPERBLADE_TEAMBLOCK_H
#define HYPERBLADE_TEAMBLOCK_H

#include "teamplayerrow.h"
#include "objectdef.h"
#include "engine/statids.h"
#include "engine/teamsblock.h"

class EngineObject;

#define TEAM_BLOCK_SLOTS 10
#define TEAM_BLOCK_SEATS 2

// One player's 49 match stats as the bench holds them.
struct TeamStatRow
{
    float aStat[MATCH_STAT_COUNT];
};

struct TeamBlock
{
    TeamBlock();

    ~TeamBlock();

    const char* GetName() const
    {
        return szName;
    }

    // The roster entry is a pointer to a record whose first field is the name.
    const char* GetRosterName(int nIndex) const
    {
        return apDefs[nIndex]->m_pszName;
    }

    float GetDamageRating(int nRow) const
    {
        return aRows[nRow].aStat[RATING_DAMAGE];
    }

    char szPath[0x20];        // 0x000
    char szName[0x20];        // 0x020
    char szEnvironment[0x20]; // 0x040
    char szDromeExt[0x20];    // 0x060
    char szIdLogo[0x20];      // 0x080
    char szTsLogo[0x20];      // 0x0a0
    char szTsText[0x20];      // 0x0c0
    char szTsTitle[0x20];     // 0x0e0
    char szTsAnim[0x20];      // 0x100
    // The roster's definitions. src/engine/objectdef.h declares their fields.
    class ObjectDef*    apDefs[TEAM_BLOCK_SLOTS];      // 0x120
    class EngineObject* apOnIce[TEAM_BLOCK_SLOTS];     // 0x148
    TeamStatRow         aStatRows[TEAM_ROW_COUNT];     // 0x170
    TeamPlayerRow       aRows[TEAM_ROW_COUNT];         // 0x480
    int                 aSeat[TEAM_BLOCK_SEATS];       // 0x870
    int                 aReserveSince[TEAM_ROW_COUNT]; // 0x878
};

static_assert(sizeof(TeamStatRow) == 0xc4);

inline const char* TeamsBlock::GetTeamPath(int nAt) const
{
    if ((short)nAt < m_wTeamCount)
    {
        return m_ppTeams[(short)nAt]->szPath;
    }

    return nullptr;
}

inline const char* TeamsBlock::GetTeamEnvironment(int nAt) const
{
    if ((short)nAt < m_wTeamCount)
    {
        return m_ppTeams[(short)nAt]->szEnvironment;
    }

    return nullptr;
}
#endif
