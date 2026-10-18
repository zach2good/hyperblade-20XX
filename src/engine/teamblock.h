// The 0x888-byte team block both team pointers name.

#ifndef HYPERBLADE_TEAMBLOCK_H
#define HYPERBLADE_TEAMBLOCK_H

#include "teamplayerrow.h"
#include "common/clock.h"
#include "objectdef.h"
#include "engine/statids.h"
#include "engine/teamsblock.h"

class EngineObject;

constexpr auto kTeamBlockSlots = 10;
constexpr auto kTeamBlockSeats = 2;

// One player's 49 match stats as the bench holds them.
struct TeamStatRow
{
    float aStat[kMatchStatCount];
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
        return aRows[nRow].aStat[kRatingDamage];
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
    class ObjectDef*    apDefs[kTeamBlockSlots];      // 0x120
    class EngineObject* apOnIce[kTeamBlockSlots];     // 0x148
    TeamStatRow         aStatRows[kTeamRowCount];     // 0x170
    TeamPlayerRow       aRows[kTeamRowCount];         // 0x480
    int                 aSeat[kTeamBlockSeats];       // 0x870
    Clock::Milliseconds aReserveSince[kTeamRowCount]; // 0x878
};

static_assert(sizeof(TeamStatRow) == 0xc4);

inline const char* TeamsBlock::GetTeamPath(int nAt) const
{
    if (static_cast<short>(nAt) < m_wTeamCount)
    {
        return m_ppTeams[static_cast<short>(nAt)]->szPath;
    }

    return nullptr;
}

inline const char* TeamsBlock::GetTeamEnvironment(int nAt) const
{
    if (static_cast<short>(nAt) < m_wTeamCount)
    {
        return m_ppTeams[static_cast<short>(nAt)]->szEnvironment;
    }

    return nullptr;
}
#endif
