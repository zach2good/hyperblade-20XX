#include "common/compat.h"
#include "animregistry.h"
#include "animclipset.h"
#include "animset.h"
#include "sound/soundmanager.h"
#include "globals.h"
#include "common/pathstring.h"
#include "animdb.h"
#include "animglobals.h"
#include "animclipentry.h"
#include "common/animnamedref.h"
#include "app/gameapp.h"
#include <cstring>
#include <cstdio>
#include "animnode.h"
#include "anim/animfile.h"
#include "engine/messageids.h"
#include <SDL3/SDL.h>

// FUNCTION: HYPERX 0x00416f70
AnimRegistry::~AnimRegistry()
{
    for (int i = 0; i < m_nSetCount; i++)
    {
        delete m_apSets[i];
        m_apSets[i] = nullptr;
    }

    // The original frees a lone set's array with scalar delete.
    if (m_nSetCount == 1)
    {
        delete m_apSets;
    }
    else
    {
        delete[] m_apSets;
    }

    if (m_pScripts != nullptr)
    {
        delete m_pScripts;
        m_pScripts = nullptr;
    }
}

// The parse runs on m_pScripts and this one is only the owner.

// FUNCTION: HYPERX 0x00416fd0
void AnimRegistry::LoadFromPath(const char* pszPath)
{
    const char* pszJoined;

    m_pScripts->ParseScript((pszJoined = String::Concat(g_gameApp.m_strExeDir.m_str,
                                                        pszPath)
                                             .m_pchData),
                            this,
                            1);
}

// Both name lookups use the multibyte compare, not _stricmp.
// FUNCTION: HYPERX 0x00417040
AnimDB* AnimRegistry::FindAnimByNamePtr(const char* pszName)
{
    for (short iAnim = 0; iAnim < m_pScripts->m_nCount; iAnim++)
    {
        if (SDL_strcasecmp(pszName, m_pScripts->GetEntry(iAnim)->GetName()) == 0)
        {
            return m_pScripts->GetEntry(iAnim);
        }
    }

    return nullptr;
}

// FUNCTION: HYPERX 0x004170a0
short AnimRegistry::FindAnimByNameIdx(const char* pszName)
{
    for (short iAnim = 0; iAnim < m_pScripts->m_nCount; iAnim++)
    {
        if (SDL_strcasecmp(pszName, m_pScripts->GetEntry(iAnim)->GetName()) == 0)
        {
            return iAnim;
        }
    }

    return -1;
}

// FUNCTION: HYPERX 0x004170f0
AnimClipSet* AnimRegistry::FindOrLoad(const char* pszName)
{
    AnimClipSet* const pSet = FindInCache(pszName);

    if (pSet != nullptr)
    {
        return pSet;
    }

    return AnimFile::Open(pszName, this, 0, AnimSet::LoadIntoRegistry);
}

// Parses the name with sscanf once per slot; compares case-insensitively up to 32 characters.
// FUNCTION: HYPERX 0x00417120
AnimClipSet* AnimRegistry::FindInCache(const char* pszName)
{
    char szParsed[32];

    AnimClipSet* pFound = nullptr;

    for (int i = 0; i < m_nSetCount; i++)
    {
        if (m_apSets[i] != nullptr)
        {
            sscanf(pszName, g_pszAnimNameFormat, szParsed);

            const char* const pszSlot = m_apSets[i]->m_pszName;

            if (SDL_strncasecmp(szParsed, pszSlot, 32) == 0)
            {
                pFound = m_apSets[i];
                break;
            }
        }
    }

    return pFound;
}

// The first walk stops at the first empty slot, so a hole in the array ends the bind.
// FUNCTION: HYPERX 0x004171a0
AnimDB* AnimRegistry::BindAnimSets(AnimDB* pDb)
{
    short i;

    for (i = 0; i < pDb->GetNodeCount(); i++)
    {
        AnimNode* const pNode = pDb->GetNode(i);

        if (pNode == nullptr)
        {
            break;
        }

        AnimClipSet* const pSet = FindOrLoad(pNode->m_szName);

        pNode->m_pSet = pSet;

        if (pSet != nullptr)
        {
            const short nFrames = pSet->m_wEndFrame;

            if (pNode->m_wEndFrame > nFrames)
            {
                pNode->m_wEndFrame = nFrames;
            }
        }
    }

    for (i = 0; i < pDb->GetEntryCount(); i++)
    {
        AnimClipEntry* const pEntry = pDb->GetEntry(i);

        if (pEntry->m_nMessage == MSG_SYNC_FRAME)
        {
            AnimDB* const pFound = FindAnimByNamePtr(pEntry->m_pRef->m_pszName);

            pEntry->SyncCheck(pFound);
        }
    }

    return pDb;
}
