#include "common/compat.h"
#include "soundmanager.h"
#include "common/fileobj.h"
#include "globals.h"
#include "common/pathstring.h"
#include <new.h>
#include <stdio.h>
#include <string.h>
#include "app/gameapp.h"
#ifdef HYPERX_V100
#include <stdlib.h>
#endif

// FUNCTION: HYPERX100 0x004698a0
// FUNCTION: HYPERX 0x004491c0
SoundManager::SoundManager()
{
    m_nUnusedA = 0;
    m_wUnusedA = 0;

    m_nSoundCount = 0;
    m_apSounds    = 0;
    m_nUnusedB    = 0;

    for (int iSlot = 0; iSlot < SOUNDMGR_SLOTS; iSlot++)
    {
        m_awGroupFirst[iSlot]   = SOUNDMGR_NO_SOUND;
        m_awGroupCount[iSlot]   = 0;
        m_anGroupSamples[iSlot] = 0;
    }
}

#define SOUND_MESSAGE_MAX 0x50
#define SOUND_NAME_MAX    0x20

#define COMMENT_CHAR '#'

// Milliseconds to samples at 44100 Hz.
#define SAMPLES_PER_MS 44.1

// No group read yet.
#define GROUP_NONE (-1)

// FUNCTION: HYPERX100 0x00469900
// FUNCTION: HYPERX 0x00449210
void SoundManager::FreeEntries()
{
    for (short i = 0; i < m_nSoundCount; i++)
    {
        char** ppName = &m_apSounds[i]->m_pszName;

        if (*ppName != 0)
        {
            delete[] *ppName;
            *ppName = 0;
        }

        delete m_apSounds[i];
    }

    delete[] m_apSounds;
}

// The first line is a header; both walks stop at the first blank line, not at the end of file.
// FUNCTION: HYPERX100 0x00469990
// FUNCTION: HYPERX 0x00449280
unsigned short SoundManager::Load(const char* pszListName)
{
    int        nLongest = 0;
    int        nLines   = 0;
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszListName).m_pchData, MAX_PATH);
    char       szName[SOUND_NAME_MAX];
    char       szMessage[SOUND_MESSAGE_MAX];
    char*      pLine;
    int        nGroup;
    // Also the header's sample rate, which nothing reads.
    int nScanned;
    int nDuration;
    int nFlag;

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) == 0)
    {
        // STRING: HYPERX 0x004c28a8
        sprintf(szMessage, "Cannot open sound list file %s!\n", pszListName);

        throw szMessage;
    }

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine != COMMENT_CHAR)
        {
            nLines++;
        }
    }

    m_nSoundCount = (short)(nLines - 1);

    m_apSounds = new SoundEntry*[m_nSoundCount];

    memset(m_apSounds, 0, m_nSoundCount * sizeof(SoundEntry*));

    file.m_file.SeekRelative(0, SEEK_SET);

    short nGroupFirst = 0;
    short nIndex      = 0;
    int   nGroupPrev  = GROUP_NONE;

    file.m_file.GetNextLine(&pLine);

    // STRING: HYPERX 0x004c28a0
    sscanf(pLine, "%d %s ", &nScanned, szName);

    while (file.m_file.GetNextLine(&pLine) > 0)
    {
        if (*pLine == COMMENT_CHAR)
        {
            continue;
        }

        // STRING: HYPERX 0x004c2894
        nScanned = sscanf(pLine, "%s %d %d %d", szName, &nDuration, &nGroup, &nFlag);

        if (nScanned == 0 || nScanned == -1)
        {
            continue;
        }

        m_apSounds[nIndex] = new SoundEntry;

        memset(m_apSounds[nIndex], 0, sizeof(SoundEntry));

        m_apSounds[nIndex]->m_nIndex = nIndex;

        char** ppName = &m_apSounds[nIndex]->m_pszName;

        *ppName = strcpy(new char[strlen(szName) + 1], szName);

        m_apSounds[nIndex]->m_nUnusedA    = 0;
        m_apSounds[nIndex]->m_nPan        = 0;
        m_apSounds[nIndex]->m_nVolume     = 0;
        m_apSounds[nIndex]->m_nGroup      = nGroup;
        m_apSounds[nIndex]->m_nStartedMs  = 0;
        m_apSounds[nIndex]->m_nPlayState  = SOUND_STATE_IDLE;
        m_apSounds[nIndex]->m_nDurationMs = nDuration;

        if (nScanned >= 4)
        {
            m_apSounds[nIndex]->m_nLoadMode = nFlag;
        }
        else
        {
            m_apSounds[nIndex]->m_nLoadMode = 0;
        }

        if (nGroup < SOUNDMGR_SLOTS && nGroupPrev != nGroup)
        {
            m_awGroupFirst[nGroup] = nIndex;

            if (nGroupPrev >= 0)
            {
                m_awGroupCount[nGroupPrev] = (short)(nIndex - nGroupFirst);

                nGroupFirst = nIndex;

                m_anGroupSamples[nGroupPrev] = (int)(nLongest * SAMPLES_PER_MS);

                nLongest = 0;
            }

            nGroupPrev = nGroup;
        }

        m_apSounds[nIndex]->m_bPannable = 0;

        nIndex++;

        if (nLongest < nDuration)
        {
            nLongest = nDuration;
        }
    }

    m_awGroupCount[nGroupPrev]   = (short)(nIndex - nGroupFirst);
    m_anGroupSamples[nGroupPrev] = (int)(nLongest * SAMPLES_PER_MS);

    return nIndex;
}

// Signed and shared by every group, so two groups picked in one frame get different halves.
// GLOBAL: HYPERX 0x004ce994
extern short g_wSoundSequence;

// Alternates between a group's first two sounds; a negative counter returns no sound.
// FUNCTION: HYPERX100 0x00469f10
// FUNCTION: HYPERX 0x00449680
SoundEntry* SoundManager::PickFromGroup(int nGroup)
{
    int nIndex = g_wSoundSequence % 2 + m_awGroupFirst[nGroup];

    g_wSoundSequence++;

    if (nIndex >= 0)
    {
        const int nFirst = m_awGroupFirst[nGroup];

        if (nIndex >= m_awGroupCount[nGroup] + nFirst)
        {
            nIndex = nFirst;
        }

        return m_apSounds[nIndex];
    }

    return 0;
}

#ifdef HYPERX_V100
// The original's list of candidates holds two, and a group with more unplayed sounds overruns it.
#define SOUNDMGR_PICK_LIMIT 2

// A play state no SOUND_STATE_ value names; a sound has it until it first plays.
#define SOUND_STATE_UNPLAYED 0

// A sound in a group past this ends the walk with no answer.
#define SOUNDMGR_GROUP_LIMIT 0x299

// FUNCTION: HYPERX100 0x00469f80
SoundEntry* SoundManager::PickUnplayedFromGroup(int nGroup)
{
    SoundEntry* apCandidates[SOUNDMGR_PICK_LIMIT];
    short       nCandidates = 0;
    short       nIndex      = m_awGroupFirst[nGroup];
    const short nEnd        = m_awGroupCount[nGroup] + nIndex;

    if (nIndex < 0)
    {
        return 0;
    }

    for (; nIndex < nEnd; nIndex++)
    {
        SoundEntry* const pSound = m_apSounds[nIndex];

        if (pSound->m_nPlayState == SOUND_STATE_UNPLAYED)
        {
            apCandidates[nCandidates++] = pSound;
        }
    }

    if (nCandidates != 0)
    {
        return apCandidates[(short)(rand() * nCandidates / (RAND_MAX + 1))];
    }

    return FindResidentInGroup(nGroup);
}

// FUNCTION: HYPERX100 0x0046a040
SoundEntry* SoundManager::FindResidentInGroup(int nGroup)
{
    short       nIndex = m_awGroupFirst[nGroup];
    const short nEnd   = m_awGroupCount[nGroup] + nIndex;

    if (nIndex < 0)
    {
        // STRING: HYPERX100 0x00513c88
        g_gameApp.Error("No resident sound");

        return 0;
    }

    for (; nIndex < nEnd; nIndex++)
    {
        const int nState = m_apSounds[nIndex]->m_nPlayState;

        if (nState == SOUND_STATE_UNPLAYED || nState == SOUND_STATE_STOPPED)
        {
            return m_apSounds[nIndex];
        }
    }

    g_gameApp.Error("No resident sound");

    return 0;
}

// FUNCTION: HYPERX100 0x0046a0e0
SoundEntry* SoundManager::FindNewestStopped()
{
    SoundEntry*  pNewest   = 0;
    unsigned int nNewestMs = 0;

    for (short nIndex = 0; nIndex < m_nSoundCount; nIndex++)
    {
        SoundEntry* const pSound = m_apSounds[nIndex];

        if (pSound->m_nGroup > SOUNDMGR_GROUP_LIMIT)
        {
            return 0;
        }

        if (pSound->m_nPlayState == SOUND_STATE_STOPPED && pSound->m_nStartedMs > nNewestMs)
        {
            pNewest   = pSound;
            nNewestMs = pSound->m_nStartedMs;
        }
    }

    return pNewest;
}

// FUNCTION: HYPERX100 0x0046a180
SoundEntry* SoundManager::FindOldestIdleInGroup(int nGroup)
{
    SoundEntry*  pOldest   = 0;
    short        nIndex    = m_awGroupFirst[nGroup];
    const short  nEnd      = m_awGroupCount[nGroup] + nIndex;
    unsigned int nOldestMs = 0xffffffff;

    for (; nIndex < nEnd; nIndex++)
    {
        SoundEntry* const pSound = m_apSounds[nIndex];

        if (pSound->m_nStartedMs < nOldestMs && pSound->m_nPlayState == SOUND_STATE_IDLE)
        {
            pOldest   = pSound;
            nOldestMs = pSound->m_nStartedMs;
        }
    }

    return pOldest;
}
#endif
