#include "common/compat.h"
#include "common/fileobj.h"
#include "globals.h"
#include "common/pathstring.h"
#include "jumbotextlist.h"
#include <cstdio>
#include <cstring>
#include <new>
#include <cstdlib>
#include <ctime>
#include "app/gameapp.h"

#define JUMBO_MESSAGE_MAX 128

// FUNCTION: HYPERX 0x0046bf50
JumboTextList* JumboTextList::Init()
{
    m_nCount   = 0;
    m_ppszText = nullptr;

    return this;
}

// A null row is skipped.
// FUNCTION: HYPERX 0x0046bf60
void JumboTextList::FreeAll()
{
    for (int iRow = 0; iRow < m_nCount; iRow++)
    {
        char** ppRow = &m_ppszText[iRow];

        if (*ppRow != nullptr)
        {
            delete[] *ppRow;

            *ppRow = nullptr;
        }
    }
}

// Reseeds from the clock every call, so calls within a second draw the same row.
// FUNCTION: HYPERX 0x0046bfa0
int JumboTextList::GetRandom(char* pszOut, int nMax)
{
    srand((unsigned int)time(nullptr));

    const char* pszRow = m_ppszText[rand() % m_nCount];

    if ((int)strlen(pszRow) < nMax)
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
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData, MAX_PATH_LENGTH);
    char*      pLine;
    char*      pszText;
    char       szNoString[JUMBO_MESSAGE_MAX];

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) == 0)
    {
        // STRING: HYPERX 0x004c4714
        g_gameApp.Fatal("ERROR OPENING JUMBOTRON ADDS LIST FILE ");

        return 0;
    }

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine != '#')
        {
            m_nCount++;
        }
    }

    m_ppszText = new char*[m_nCount];

    memset(m_ppszText, 0, m_nCount * sizeof(char*));

    file.m_file.SeekRelative(0, SEEK_SET);
    m_nCount = 0;

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine != '#')
        {
            char* const pszSource = pLine;

            if (pLine != nullptr)
            {
                pszText = strcpy(new char[strlen(pszSource) + 1], pszSource);
            }
            else
            {
                // The format string's marker is in messagequeue.cpp.
                sprintf(szNoString, "!EEK no string on line: %d\n", m_nCount);

                pszText = strcpy(new char[strlen(szNoString) + 1], szNoString);
            }

            m_ppszText[m_nCount] = pszText;
            m_nCount++;
        }
    }

    file.m_file.CloseAndTest();

    return 1;
}
