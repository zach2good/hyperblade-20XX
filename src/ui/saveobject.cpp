#include "common/compat.h"
#include "torchbitmaps.h"
#include "globals.h"
#include "engine/gameengine.h"
#include "engine/gameoptions.h"
#include "engine/engineglobals.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "common/fileobj.h"
#include "common/pathstring.h"
#include <cstring>
#include <cstdlib>
#include "app/gameapp.h"
#include "saveobject.h"
#include "engine/teamsblock.h"

// The bracket is filled with minus one after the four fields are set.

// FUNCTION: HYPERX 0x00448d50
SaveObject::SaveObject(short nHiddenTeamA, short nHiddenTeamB)
{
    m_wHiddenTeamA       = nHiddenTeamA;
    m_wHiddenTeamB       = nHiddenTeamB;
    m_wBracketPos        = 0;
    m_nAllTeamsAvailable = 0;

    memset(m_aBracket, -1, sizeof(m_aBracket));
}

// FUNCTION: HYPERX 0x00448d90
short SaveObject::WrapToNextValidTeam(short nTeam, int nDirection)
{
    const int nTeamCount = g_pGameEngine->m_pTeamTable->GetTeamCount();

    while (m_nAllTeamsAvailable != 1)
    {
        if (nTeam != m_wHiddenTeamA && nTeam != m_wHiddenTeamB)
        {
            return nTeam;
        }

        if (nDirection == 1)
        {
            nTeam++;

            if (nTeam > nTeamCount - 1)
            {
                nTeam = 0;
            }
        }
        else
        {
            nTeam--;

            if (nTeam < 0)
            {
                nTeam = static_cast<short>(nTeamCount - 1);
            }
        }
    }

    return nTeam;
}

// FUNCTION: HYPERX 0x00448e00
short SaveObject::GetBracketPos() const
{
    return m_wBracketPos;
}

// Advance steps the row and then reads; peek only reads.
// FUNCTION: HYPERX 0x00448e10
short SaveObject::BracketIterPeek()
{
    g_nDromeTeamAway = m_aBracket[g_nDromeTeamHome][m_wBracketPos];

    return static_cast<short>(g_nDromeTeamAway);
}

// FUNCTION: HYPERX 0x00448e30
short SaveObject::GetHomeTeam() const
{
    return g_pGameEngine->m_nTeamHomeIndex;
}

// FUNCTION: HYPERX 0x00448e40
short SaveObject::BracketIterAdvance()
{
    m_wBracketPos++;

    g_nDromeTeamAway = m_aBracket[g_nDromeTeamHome][m_wBracketPos];

    return static_cast<short>(g_nDromeTeamAway);
}

// FUNCTION: HYPERX 0x00448e70
void SaveObject::ValidateSetHomeTeam(short nTeam, int nFlag)
{
    g_nDromeTeamHome = WrapToNextValidTeam(nTeam, nFlag);
}

// FUNCTION: HYPERX 0x00448e90
void SaveObject::BracketIterReset()
{
    SetBracketPos(0);
}

// FUNCTION: HYPERX 0x00448ea0
void SaveObject::SetBracketPos(short nPos)
{
    m_wBracketPos = nPos;
}

// FUNCTION: HYPERX 0x00448eb0
void SaveObject::SetAllTeamsAvailable(int nAvailable)
{
    m_nAllTeamsAvailable = nAvailable;

    if (g_pGameOptions != nullptr && nAvailable != 0)
    {
        g_pGameOptions->WriteInt(g_pszMiscSection, g_pszPotatoeKey, 1);
    }
}

// Twenty ids a round is a ceiling. A line with no space is the only parse error.

// FUNCTION: HYPERX 0x00448ee0
unsigned int SaveObject::ParseGauntletConfig(const char* pszFileName)
{
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData,
                       kMaxPathLength);
    char*      pLine;

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) == 0)
    {
        // STRING: HYPERX 0x004c2870
        g_gameApp.Fatal("ERROR OPENING GAUNTLET TEAM FILE ");

        return 0;
    }

    file.m_file.SeekRelative(0, FileSeekOrigin::kStart);
    auto nRound = 0;

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine != '#')
        {
            auto* pSpace = strchr(pLine, ' ');

            if (pSpace == nullptr)
            {
                // STRING: HYPERX 0x004c283c
                g_gameApp.Error("Gauntlet Error--Opponent Team errors on line: %d!",
                                nRound);

                return 0;
            }

            for (int nSlot = 0; nSlot < kGauntletRoundSlots && pSpace != nullptr; nSlot++)
            {
                pSpace++;
                const auto nValue         = static_cast<short>(atoi(pSpace));
                pSpace                    = strchr(pSpace, ' ');
                m_aBracket[nRound][nSlot] = nValue;
            }

            nRound++;
        }
    }

    file.m_file.CloseAndTest();

    if (g_pGameOptions != nullptr && g_pGameOptions->ReadInt(g_pszMiscSection, g_pszPotatoeKey, 0) != 0)
    {
        m_nAllTeamsAvailable = 1;
    }

    return 1;
}

const char* g_pszMiscSection = "Misc";
const char* g_pszPotatoeKey  = "Potatoe";
