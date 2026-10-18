#include "common/compat.h"
#include "soundmanager.h"
#include "common/fileobj.h"
#include "globals.h"
#include "common/pathstring.h"
#include <new>
#include <cstdio>
#include <cstring>
#include "app/gameapp.h"

// FUNCTION: HYPERX 0x004491c0
SoundManager::SoundManager()
{
    m_nUnusedA = 0;
    m_wUnusedA = 0;

    m_nSoundCount = 0;
    m_apSounds    = nullptr;
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

// FUNCTION: HYPERX 0x00449210
void SoundManager::FreeEntries()
{
    for (short i = 0; i < m_nSoundCount; i++)
    {
        char** ppName = &m_apSounds[i]->m_pszName;

        if (*ppName != nullptr)
        {
            delete[] *ppName;
            *ppName = nullptr;
        }

        delete m_apSounds[i];
    }

    delete[] m_apSounds;
}

// The first line is a header; both walks stop at the first blank line, not at the end of file.
// FUNCTION: HYPERX 0x00449280
unsigned short SoundManager::Load(const char* pszListName)
{
    int        nLongest = 0;
    int        nLines   = 0;
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszListName).m_pchData, MAX_PATH_LENGTH);
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

    return nullptr;
}
