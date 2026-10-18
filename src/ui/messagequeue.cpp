#include "common/compat.h"
#include "globals.h"
#include "engine/messagequeue.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "common/fileobj.h"
#include "common/pathstring.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "app/gameapp.h"
#include "engine/gameengine.h"

constexpr auto kInfobarMessageMax = 128;

// FUNCTION: HYPERX 0x00452710
MessageQueue::MessageQueue()
{
    m_expiryTick = {};
}

constexpr auto kMessageColourGreen  = 0xff00;
constexpr auto kMessageColourYellow = 0xffff00;
constexpr auto kMessageColourRed    = 0xff;

// Zero expiry means idle.
// FUNCTION: HYPERX 0x00452730
void MessageQueue::Update()
{
    int nColour;

    if (m_expiryTick != Clock::Milliseconds{})
    {
        if (g_pGameEngine->m_clock.tick > m_expiryTick)
        {
            g_pRenderer->HideMessage();

            m_expiryTick = {};
        }
    }
    else if (!m_pending.empty())
    {
        const auto& message = m_pending.front();

        switch (message.m_nLine)
        {
            case 0:
                nColour = kMessageColourGreen;
                break;

            case 1:
                nColour = kMessageColourYellow;
                break;

            default:
                nColour = kMessageColourRed;
                break;
        }

        m_expiryTick = g_pGameEngine->m_clock.tick + Clock::Milliseconds{ static_cast<std::uint32_t>(message.m_nDurationMs) };

        g_pRenderer->ShowMessage();
        g_pRenderer->SetMessageText(message.m_text.c_str(), nColour);

        m_pending.pop_front();
    }
    else
    {
        g_pRenderer->HideMessage();
    }
}

// An index past the end leaves the queue untouched.
// FUNCTION: HYPERX 0x00452850
void MessageQueue::PushFromTable(int nIndex, int nLine, int nDurationMs)
{
    if (nIndex < static_cast<int>(m_names.size()))
    {
        m_pending.push_back({ m_names[nIndex], nLine, nDurationMs });
    }
}

// FUNCTION: HYPERX 0x00452930
void MessageQueue::PushNamedFromTable(const char* pszPrefix, int nIndex, int nLine, int nDurationMs)
{
    char szName[kMaxPathLength];

    strcpy(szName, pszPrefix);

    if (nIndex >= static_cast<int>(m_names.size()))
    {
        return;
    }

    strcat(szName, m_names[nIndex].c_str());

    m_pending.push_back({ szName, nLine, nDurationMs });
}

// FUNCTION: HYPERX 0x00452a80
void MessageQueue::Push(const char* pszText, int nLine, int nDurationMs)
{
    m_pending.push_back({ pszText, nLine, nDurationMs });
}

// FUNCTION: HYPERX 0x00452b40
MessageQueue* MessageQueue::FreeAll()
{
    m_names.clear();
    m_ids.clear();

    return this;
}

// FUNCTION: HYPERX 0x00452b90
int MessageQueue::GetAt(int nIndex) const
{
    return m_ids[nIndex];
}

// "id text" per line. A malformed line becomes a complaint message rather than being dropped.
// FUNCTION: HYPERX 0x00452ba0
unsigned int MessageQueue::LoadList(const char* pszFileName)
{
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData,
                       kMaxPathLength);
    char*      pLine;
    int        nId;
    char       szNoString[kInfobarMessageMax];
    char       szNoDescription[kInfobarMessageMax];

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) == 0)
    {
        // STRING: HYPERX 0x004c3030
        g_gameApp.Fatal("ERROR OPENING INFOBAR LIST FILE ");

        return 0;
    }

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine != '#')
        {
            auto* pSpace = strchr(pLine, ' ');

            if (pSpace != nullptr)
            {
                pSpace++;
                nId = atoi(pSpace);

                auto* const pText = strchr(pSpace, ' ');

                if (pText != nullptr)
                {
                    auto* const pMessage = pText + 1;

                    m_names.emplace_back(pMessage);
                }
                else
                {
                    // STRING: HYPERX 0x004c3014
                    sprintf(szNoString, "!EEK no string on line: %d\n", static_cast<int>(m_names.size()));

                    m_names.emplace_back(szNoString);
                }
            }
            else
            {
                // STRING: HYPERX 0x004c2ff0
                sprintf(szNoDescription, "!EEK empty description on line: %d\n", static_cast<int>(m_names.size()));

                m_names.emplace_back(szNoDescription);

                nId = 0;
            }

            m_ids.push_back(nId);
        }
    }

    file.m_file.CloseAndTest();

    return 1;
}

// FUNCTION: HYPERX 0x00452ee0
void MessageQueue::ProcessAndClear()
{
    g_pRenderer->HideMessage();

    m_expiryTick = {};

    m_pending.clear();
}
