#include "common/compat.h"
#include "globals.h"
#include "engine/messagequeue.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "common/fileobj.h"
#include "common/pathstring.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "app/gameapp.h"
#include "engine/gameengine.h"

#define INFOBAR_MESSAGE_MAX 128

// src/engine/game.cpp allocates this as a sixteen-byte block.
// FUNCTION: HYPERX 0x00452710
MessageQueue::MessageQueue()
{
    m_wCount      = 0;
    m_nExpiryTick = 0;
    m_pnIds       = nullptr;
    m_ppNames     = nullptr;
}

#define MESSAGE_COLOUR_GREEN  0xff00
#define MESSAGE_COLOUR_YELLOW 0xffff00
#define MESSAGE_COLOUR_RED    0xff

// Zero expiry means idle. Releasing the text can run arbitrary code, so the head is re-read.
// FUNCTION: HYPERX 0x00452730
void MessageQueue::Update()
{
    int nColour;

    if (m_nExpiryTick != 0)
    {
        if (g_pGameEngine->m_clock.nTickMs > m_nExpiryTick)
        {
            g_pRenderer->HideMessage();

            m_nExpiryTick = 0;
        }
    }
    else if (g_pMessageHead != nullptr)
    {
        switch (g_pMessageHead->m_nLine)
        {
            case 0:
                nColour = MESSAGE_COLOUR_GREEN;
                break;

            case 1:
                nColour = MESSAGE_COLOUR_YELLOW;
                break;

            default:
                nColour = MESSAGE_COLOUR_RED;
                break;
        }

        m_nExpiryTick = g_pGameEngine->m_clock.nTickMs + g_pMessageHead->m_nDurationMs;

        g_pRenderer->ShowMessage();
        g_pRenderer->SetMessageText(g_pMessageHead->m_pszText, nColour);

        MessageNode* const pNode = g_pMessageHead;
        MessageNode*       pHead = pNode;

        if (pNode != nullptr)
        {
            if (pNode->m_pszText != nullptr)
            {
                delete[] pNode->m_pszText;
                pNode->m_pszText = nullptr;
                pHead            = g_pMessageHead;
            }

            MessageNode* pWalk = g_pMessageHead;
            MessageNode* pPrev = nullptr;

            for (; pWalk != nullptr; pWalk = pWalk->m_pNext)
            {
                if (pWalk == pNode)
                {
                    if (pPrev != nullptr)
                    {
                        pPrev->m_pNext = pNode->m_pNext;
                        pHead          = g_pMessageHead;
                    }

                    if (pNode == pHead)
                    {
                        g_pMessageHead = pHead->m_pNext;
                    }

                    if (pNode == g_pMessageTail)
                    {
                        g_pMessageTail = pPrev;
                    }

                    break;
                }

                pPrev = pWalk;
            }

            delete pNode;
        }
    }
    else
    {
        g_pRenderer->HideMessage();
    }
}

// An index past the end, an empty entry or a failed allocation leaves the queue untouched.
// FUNCTION: HYPERX 0x00452850
void MessageQueue::PushFromTable(int nIndex, int nLine, int nDurationMs)
{
    if (nIndex < m_wCount && m_ppNames[nIndex] != nullptr)
    {
        new MessageNode(m_ppNames[nIndex], nLine, nDurationMs);
    }
}

// FUNCTION: HYPERX 0x00452930
void MessageQueue::PushNamedFromTable(const char* pszPrefix, int nIndex, int nLine, int nDurationMs)
{
    char szName[MAX_PATH_LENGTH];

    strcpy(szName, pszPrefix);

    if (nIndex >= m_wCount)
    {
        return;
    }

    const char* pszEntry = m_ppNames[nIndex];

    if (pszEntry == nullptr)
    {
        return;
    }

    strcat(szName, pszEntry);

    new MessageNode(szName, nLine, nDurationMs);
}

// FUNCTION: HYPERX 0x00452a80
void MessageQueue::Push(const char* pszText, int nLine, int nDurationMs)
{
    new MessageNode(pszText, nLine, nDurationMs);
}

// The block pointer is not cleared, so the queue keeps a pointer to freed memory.
// FUNCTION: HYPERX 0x00452b40
MessageQueue* MessageQueue::FreeAll()
{
    for (int iName = 0; iName < m_wCount; iName++)
    {
        const char** ppName = &m_ppNames[iName];

        if (*ppName != nullptr)
        {
            delete[] *ppName;

            *ppName = nullptr;
        }
    }

    if (m_pnIds != nullptr)
    {
        delete[] m_pnIds;
    }

    return this;
}

// FUNCTION: HYPERX 0x00452b90
int MessageQueue::GetAt(int nIndex) const
{
    return m_pnIds[nIndex];
}

// "id text" per line. A malformed line becomes a complaint message rather than being dropped.
// FUNCTION: HYPERX 0x00452ba0
unsigned int MessageQueue::LoadList(const char* pszFileName)
{
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData,
                       MAX_PATH_LENGTH);
    char*      pLine;
    char*      pszText;
    int        nId;
    char       szNoString[INFOBAR_MESSAGE_MAX];
    char       szNoDescription[INFOBAR_MESSAGE_MAX];

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) == 0)
    {
        // STRING: HYPERX 0x004c3030
        g_gameApp.Fatal("ERROR OPENING INFOBAR LIST FILE ");

        return 0;
    }

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine != '#')
        {
            m_wCount++;
        }
    }

    m_ppNames = new const char*[m_wCount];
    m_pnIds   = new int[m_wCount];

    memset(m_ppNames, 0, m_wCount * sizeof(const char*));

    file.m_file.SeekRelative(0, SEEK_SET);
    m_wCount = 0;

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine != '#')
        {
            char* pSpace = (char*)strchr(pLine, ' ');

            if (pSpace != nullptr)
            {
                pSpace++;
                nId = atoi(pSpace);

                char* const pText = (char*)strchr(pSpace, ' ');

                if (pText != nullptr)
                {
                    char* const pMessage = pText + 1;

                    pszText = strcpy(new char[strlen(pMessage) + 1], pMessage);
                }
                else
                {
                    // STRING: HYPERX 0x004c3014
                    sprintf(szNoString, "!EEK no string on line: %d\n", m_wCount);

                    pszText = strcpy(new char[strlen(szNoString) + 1], szNoString);
                }
            }
            else
            {
                // STRING: HYPERX 0x004c2ff0
                sprintf(szNoDescription, "!EEK empty description on line: %d\n", m_wCount);

                pszText = strcpy(new char[strlen(szNoDescription) + 1], szNoDescription);

                nId = 0;
            }

            m_ppNames[m_wCount] = pszText;
            m_pnIds[m_wCount]   = nId;
            m_wCount++;
        }
    }

    file.m_file.CloseAndTest();

    return 1;
}

// Freeing the text can run arbitrary code, so the head is re-read afterwards.
// FUNCTION: HYPERX 0x00452ee0
void MessageQueue::ProcessAndClear(void)
{
    MessageNode* pWalk;

    g_pRenderer->HideMessage();

    m_nExpiryTick = 0;

    while (g_pMessageHead != nullptr)
    {
        MessageNode* const pNode = g_pMessageHead;

        if (pNode != nullptr)
        {
            if (pNode->m_pszText != nullptr)
            {
                delete[] pNode->m_pszText;
                pNode->m_pszText = nullptr;
            }

            MessageNode* pPrev = nullptr;

            for (pWalk = g_pMessageHead; pWalk != nullptr; pWalk = pWalk->m_pNext)
            {
                if (pWalk == pNode)
                {
                    if (pPrev != nullptr)
                    {
                        pPrev->m_pNext = pNode->m_pNext;
                    }

                    if (pNode == g_pMessageHead)
                    {
                        g_pMessageHead = g_pMessageHead->m_pNext;
                    }

                    if (pNode == g_pMessageTail)
                    {
                        g_pMessageTail = pPrev;
                    }

                    break;
                }

                pPrev = pWalk;
            }

            delete pNode;
        }
    }
}
