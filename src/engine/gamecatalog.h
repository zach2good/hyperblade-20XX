// v1.0.0.0's unfinished season catalogue, read from year.cat. Nothing constructs one.

#ifndef HYPERBLADE_GAMECATALOG_H
#define HYPERBLADE_GAMECATALOG_H

#ifdef HYPERX_V100

#include "common/staticassert.h"

class FileObj;

// One scheduled game, 0x18 bytes.
struct CatalogGame
{
    int nHomeTeamId; // 0x00
    int nAwayTeamId; // 0x04
    int nHomeScore;  // 0x08
    int nAwayScore;  // 0x0c
    // TODO: Nothing reads or writes these two words.
    int anUnused[2]; // 0x10
};

STATIC_ASSERT(sizeof(CatalogGame) == 0x18);

class GameCatalog
{
public:
    GameCatalog();

    void ParseGameCat(FileObj* pFile);

    // TODO: Every secret game gets the same id on both sides. Is nTeam a team or a level?
    void AssignTeams(int nTeam);

    CatalogGame* m_pGames; // 0x00
    // TODO: Only the constructor writes it.
    int m_nUnused;          // 0x04
    int m_nSecretCount;     // 0x08
    int m_nTournamentCount; // 0x0c
    int m_nSeasonCount;     // 0x10
    // The team count teams.cat gives.
    int m_nTeamCount; // 0x14
};

STATIC_ASSERT(sizeof(GameCatalog) == 0x18);

#endif

#endif
