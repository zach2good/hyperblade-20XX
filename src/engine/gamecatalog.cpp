#ifdef HYPERX_V100

#include "common/compat.h"
#include "gamecatalog.h"
#include "teamsblock.h"
#include "engineglobals.h"
#include "common/fileobj.h"
#include "common/parseutil.h"
#include "app/gameapp.h"
#include <mbstring.h>
#include <stdlib.h>
#include <string.h>

#define GAMECAT_KEYWORD_COUNT 11

// The definitions all land in the ninth record.
#define GAMECAT_WRITTEN_GAME 8

// FUNCTION: HYPERX100 0x00437dd0
GameCatalog::GameCatalog()
{
    char szPath[64];

    m_pGames           = 0;
    m_nUnused          = 0;
    m_nSecretCount     = 0;
    m_nTournamentCount = 0;
    m_nSeasonCount     = 0;
    m_nTeamCount       = 0;

    TeamsBlock* const pTeams = new TeamsBlock;

    // The count is read before the null test.
    m_nTeamCount = pTeams->GetTeamCount();

    delete pTeams;

    OwnedFile catFile;

    strcpy(szPath, g_gameApp.m_strExeDir.m_str.m_pchData);
    // STRING: HYPERX100 0x0050f824
    strcat(szPath, "year.cat");

    if (catFile.m_file.Open(szPath, FILE_MODE_READ) != 0)
    {
        ParseGameCat(&catFile.m_file);
    }

    catFile.m_file.CloseAndTest();
}

// Sets both ids of every secret game, capped at seven; each store's zero test never passes.
// FUNCTION: HYPERX100 0x00437fa0
void GameCatalog::AssignTeams(int nTeam)
{
    int nWas;
    int i;

    if (nTeam < 7)
    {
        for (i = 0; i < m_nSecretCount; i++)
        {
            m_pGames[i].nHomeTeamId = 1;
            nWas                    = m_pGames[i].nHomeTeamId;
            m_pGames[i].nHomeTeamId = nTeam;

            if (nWas == 0 && m_pGames[i].nHomeTeamId != 0)
            {
                m_pGames[i].nHomeTeamId = 1;
            }

            m_pGames[i].nAwayTeamId = 1;
            nWas                    = m_pGames[i].nAwayTeamId;
            m_pGames[i].nAwayTeamId = nTeam;

            if (nWas == 0 && m_pGames[i].nAwayTeamId != 0)
            {
                m_pGames[i].nAwayTeamId = 1;
            }
        }

        return;
    }

    for (i = 0; i < m_nSecretCount; i++)
    {
        m_pGames[i].nHomeTeamId = 7;
        nWas                    = m_pGames[i].nHomeTeamId;
        m_pGames[i].nHomeTeamId = nTeam;

        if (nWas == 0 && m_pGames[i].nHomeTeamId != 0)
        {
            m_pGames[i].nHomeTeamId = 7;
        }

        m_pGames[i].nAwayTeamId = 7;
        nWas                    = m_pGames[i].nAwayTeamId;
        m_pGames[i].nAwayTeamId = nTeam;

        if (nWas == 0 && m_pGames[i].nAwayTeamId != 0)
        {
            m_pGames[i].nAwayTeamId = 7;
        }
    }
}

// The three counts and the away score share one error message; the game index is parsed and dropped.
// FUNCTION: HYPERX100 0x00438180
void GameCatalog::ParseGameCat(FileObj* pFile)
{
    const char* apszKeyword[GAMECAT_KEYWORD_COUNT] = {
        // STRING: HYPERX100 0x0050f9e8
        "SeasonCount",
        // STRING: HYPERX100 0x0050f9d4
        "TournamentCount",
        // STRING: HYPERX100 0x0050f9c4
        "SecretCount",
        // STRING: HYPERX100 0x0050f9b4
        "StartDefine",
        // STRING: HYPERX100 0x0050f9a4
        "DefineGame",
        // STRING: HYPERX100 0x0050f998
        "EndDefine",
        // STRING: HYPERX100 0x0050f988
        "HomeTeamID",
        // STRING: HYPERX100 0x0050f978
        "AwayTeamID",
        // STRING: HYPERX100 0x0050f96c
        "gameindex",
        // STRING: HYPERX100 0x0050f960
        "HomeScore",
        // STRING: HYPERX100 0x0050f954
        "AwayScore"
    };

    char  szToken[PARSE_TOKEN_MAX];
    char* pLine;
    char* pszEnd;
    short nKeyword;
    char  bFound = 0;

    while (pFile->GetNextLine(&pLine) > 0)
    {
        // STRING: HYPERX100 0x0050f950
        if (ParseUtil::ParseToken(pLine, szToken, 0, " \t", PARSE_TOKEN_MAX, &pLine) == 0)
        {
            continue;
        }

        // STRING: HYPERX100 0x0050f940
        if (_mbsicmp((const unsigned char*)szToken, (const unsigned char*)"DefineTeam") == 0)
        {
            bFound = 1;
            break;
        }
    }

    if (bFound == 0)
    {
        return;
    }

    int nHomeTeamId = 0;
    int nAwayTeamId = 0;
    int nHomeScore  = 0;
    int nAwayScore  = 0;

    while (pLine != 0)
    {
        // STRING: HYPERX100 0x0050f93c
        if (ParseUtil::ParseToken(pLine, szToken, 0, " =\t", PARSE_TOKEN_MAX, &pLine) == 0)
        {
            return;
        }

        pLine = ParseUtil::SkipToChar(pLine, g_pszEqualsSign);

        if (pLine == 0)
        {
            return;
        }

        nKeyword = ParseUtil::FindKeyword(szToken, apszKeyword, GAMECAT_KEYWORD_COUNT);

        if (nKeyword < 0)
        {
            return;
        }

        switch (nKeyword)
        {
            case 0:
                pLine++;

                // STRING: HYPERX100 0x0050f90c
                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    goto awayScoreError;
                }

                m_nSeasonCount = strtol(szToken, &pszEnd, 10);
                break;

            case 1:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    goto awayScoreError;
                }

                m_nTournamentCount = strtol(szToken, &pszEnd, 10);
                break;

            case 2:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    goto awayScoreError;
                }

                m_nSecretCount = strtol(szToken, &pszEnd, 10);
                break;

            case 3:
                if (m_nSecretCount == 0)
                {
                    // STRING: HYPERX100 0x0050f914
                    g_gameApp.Error("Check series definition--  %s!", szToken);

                    return;
                }

                m_pGames = new CatalogGame[m_nTournamentCount + m_nSeasonCount + m_nSecretCount];
                break;

            case 4:
            case 5:
                if (nHomeTeamId != 0)
                {
                    m_pGames[GAMECAT_WRITTEN_GAME].nHomeTeamId = nHomeTeamId;
                    m_pGames[GAMECAT_WRITTEN_GAME].nAwayTeamId = nAwayTeamId;
                    m_pGames[GAMECAT_WRITTEN_GAME].nHomeScore  = nHomeScore;
                    m_pGames[GAMECAT_WRITTEN_GAME].nAwayScore  = nAwayScore;
                }

                break;

            case 6:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX100 0x0050f8e0
                    g_gameApp.Error("ParseGameCat--HomeTeamID error:  %s!", szToken);

                    return;
                }

                nHomeTeamId = strtol(szToken, &pszEnd, 10);
                break;

            case 7:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX100 0x0050f8b4
                    g_gameApp.Error("ParseGameCat--AwayTeamID error: %s!", szToken);

                    return;
                }

                nAwayTeamId = strtol(szToken, &pszEnd, 10);
                break;

            case 8:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX100 0x0050f888
                    g_gameApp.Error("ParseGameCat--gameindex error: %s!", szToken);

                    return;
                }

                strtol(szToken, &pszEnd, 10);
                break;

            case 9:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX100 0x0050f85c
                    g_gameApp.Error("ParseGameCat--HomeScore error: %s!", szToken);

                    return;
                }

                nHomeScore = strtol(szToken, &pszEnd, 10);
                break;

            case 10:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                awayScoreError:
                    // STRING: HYPERX100 0x0050f830
                    g_gameApp.Error("ParseGameCat--AwayScore error: %s!", szToken);

                    return;
                }

                nAwayScore = strtol(szToken, &pszEnd, 10);
                break;
        }
    }
}

#endif
