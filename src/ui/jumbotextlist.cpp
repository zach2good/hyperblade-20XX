#include "math/squirrel5.h"
#include "common/compat.h"
#include "common/fileobj.h"
#include "globals.h"
#include "common/pathstring.h"
#include "jumbotextlist.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "app/gameapp.h"

constexpr auto kJumboMessageMax = 128;

// FUNCTION: HYPERX 0x0046bf50
JumboTextList* JumboTextList::Init()
{
    m_rows.clear();

    return this;
}

// FUNCTION: HYPERX 0x0046bf60
void JumboTextList::FreeAll()
{
    m_rows.clear();
}

// Reseeds from the clock every call, so calls within a second draw the same row.
// FUNCTION: HYPERX 0x0046bfa0
int JumboTextList::GetRandom(char* pszOut, int nMax)
{
    Math::SeedRand(static_cast<uint32_t>(time(nullptr)));

    const char* const pszRow = m_rows[Math::Rand() % static_cast<int>(m_rows.size())].c_str();

    if (static_cast<int>(strlen(pszRow)) < nMax)
    {
        strcpy(pszOut, pszRow);

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0046c010
unsigned int JumboTextList::Load(const char* pszFileName)
{
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData, kMaxPathLength);
    char*      pLine;
    char       szNoString[kJumboMessageMax];

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) == 0)
    {
        // STRING: HYPERX 0x004c4714
        g_gameApp.Fatal("ERROR OPENING JUMBOTRON ADDS LIST FILE ");

        return 0;
    }

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine != '#')
        {
            auto* const pszSource = pLine;

            if (pLine != nullptr)
            {
                m_rows.emplace_back(pszSource);
            }
            else
            {
                // The format string's marker is in messagequeue.cpp.
                sprintf(szNoString, "!EEK no string on line: %d\n", static_cast<int>(m_rows.size()));

                m_rows.emplace_back(szNoString);
            }
        }
    }

    file.m_file.CloseAndTest();

    return 1;
}
