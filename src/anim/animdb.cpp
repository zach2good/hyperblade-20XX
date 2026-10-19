#include "common/compat.h"
#include "animdb.h"
#include "common/animnamedref.h"
#include "animclipentry.h"
#include "animnode.h"

#include "engine/messageids.h"
#include <SDL3/SDL.h>
#include <memory>

// FUNCTION: HYPERX 0x00417530
AnimDB::AnimDB(const char* pszName, const char*)
: m_pOwnedBlock(nullptr)
, m_wNodeCount(0)
, m_nEntryCount(0)
{
    SetName(pszName);
}

// FUNCTION: HYPERX 0x004175c0
AnimDB::~AnimDB() = default;

// FUNCTION: HYPERX 0x004176b0
void AnimDB::SetName(const char* pszName)
{
    if (pszName != nullptr)
    {
        SDL_strlcpy(m_szName, pszName, sizeof(m_szName));

        return;
    }

    m_szName[0] = 0;
}

// FUNCTION: HYPERX 0x004176d0
void AnimDB::AddClipEntry(unsigned short wSegment, unsigned short wTriggerFrame, const char* pszName, int nMessage, const char* pszRef, short nSyncSegment, short nSyncFrame)
{
    auto pEntry = std::make_unique<AnimClipEntry>(wSegment, wTriggerFrame, pszName, nMessage);

    if (nMessage == kMsgSyncFrame)
    {
        pEntry->m_pRef = std::make_unique<AnimNamedRef>(pszRef, nSyncSegment, nSyncFrame);
    }

    m_apEntries[m_nEntryCount] = std::move(pEntry);

    m_nEntryCount++;
}
