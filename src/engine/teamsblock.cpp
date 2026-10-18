// A bad definition or a filename of 32 characters stops the program; a 31st team ends the walk.

#include "common/compat.h"
#include "teamsblock.h"
#include "ui/widget.h"
#include "common/fileobj.h"
#include "globals.h"
#include "common/pathstring.h"
#include <new>
#include <stdlib.h>
#include <string.h>
#include "gameengine.h"
#include "app/gameapp.h"
#include "common/parseutil.h"
#include <SDL3/SDL.h>

#define COMMENT_CHAR '#'

// The catalogue holds thirty and the thirty-first is refused.
#define TEAM_MAX 30

// A record starts with its 32-byte filename, followed by the name the checksum reads.
#define TEAM_FILENAME_MAX 32

// FUNCTION: HYPERX 0x004847d0
TeamsBlock::TeamsBlock()
{
    TeamBlock* apTeams[TEAM_MAX];
    OwnedFile  catFile;
    char       szTeamFilename[PARSE_TOKEN_MAX];
    char*      pLine;
    short      nTeams = 0;
    // STRING: HYPERX 0x004c8378
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, "teams.cat").m_pchData, MAX_PATH_LENGTH);

    if (catFile.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0)
    {
        while (catFile.m_file.GetNextLine(&pLine) > 0)
        {
            if (pLine != nullptr
                // STRING: HYPERX 0x004c7324
                && ParseUtil::ParseToken(pLine, szTeamFilename, nullptr, " \t\n\r", (short)strlen(pLine), &pLine) != 0 && szTeamFilename[0] != COMMENT_CHAR)
            {
                if (nTeams == TEAM_MAX)
                {
                    g_gameApp.Error(
                        // STRING: HYPERX 0x004c82c4
                        "More than max allowed teams ( %d ) in catalog!",
                        TEAM_MAX);

                    break;
                }

                OwnedFile htmFile;

                if (strlen(szTeamFilename) >= TEAM_FILENAME_MAX)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c835c
                        "Team filename to long %s!",
                        szTeamFilename);
                }

                strPath.m_str.AssignFromString(
                    String::Concat(g_gameApp.m_strExeDir.m_str, szTeamFilename));

                if (htmFile.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0)
                {
                    TeamBlock* const pTeam = g_pGameEngine->ParseTeamHTMFile(&htmFile.m_file);

                    apTeams[nTeams] = pTeam;

                    if (pTeam == nullptr)
                    {
                        htmFile.m_file.CloseAndTest();
                        catFile.m_file.CloseAndTest();

                        g_gameApp.Fatal(
                            // STRING: HYPERX 0x004c8338
                            "Team definition error in file %s!",
                            szTeamFilename);
                    }

                    nTeams++;

                    strcpy(pTeam->szPath, szTeamFilename);

                    htmFile.m_file.CloseAndTest();
                }
                else
                {
                    htmFile.m_file.CloseAndTest();
                    catFile.m_file.CloseAndTest();

                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c82f4
                        "team definition file could not be opened while building catalog %s!",
                        szTeamFilename);
                }
            }
        }

        catFile.m_file.CloseAndTest();

        if (nTeams < 1)
        {
            g_gameApp.Fatal(
                // STRING: HYPERX 0x004c82a8
                "Error: empty team catalog!");
        }

        m_wTeamCount = nTeams;

        m_ppTeams = new TeamBlock*[nTeams];

        memcpy(m_ppTeams, apTeams, nTeams * sizeof(TeamBlock*));

        ComputeTeamNameChecksum();
    }
    else
    {
        g_gameApp.ReportLastError();

        g_gameApp.OnShutdown();
    }
}

// Three characters of every lowercased name, summed; the low byte of the total is dropped.

// FUNCTION: HYPERX 0x00484b30
int TeamsBlock::ComputeTeamNameChecksum()
{
    char* pszName;
    char* pszLower;

    for (short i = 0; i < m_wTeamCount; i++)
    {
        if (i < m_wTeamCount)
        {
            pszName = m_ppTeams[i]->szName;
        }
        else
        {
            pszName = nullptr;
        }

        pszLower = (char*)SDL_strlwr(Str::Duplicate(pszLower, pszName));

        m_nChecksum += (char)(pszLower[2] + pszLower[1]) ^ (pszLower[0] << 8);

        // The original tests before it frees.
        if (pszLower != nullptr)
        {
            delete pszLower;
        }
    }

    m_abChecksum[0] = 0;

    return m_nChecksum;
}
