#include "common/compat.h"
#include "soundchannel.h"
#include "globals.h"
#include "app/gameapp.h"
#include "common/pathstring.h"
#include <string.h>
#include <windows.h>
#include "sound/soundids.h"
#include "sound/mixer.h"

Mixer g_mixerCompactDisc;
Mixer g_mixerWaveOut;

#define SOUND_LOCK_RETRIES 2

// FUNCTION: HYPERX 0x0048fcc0
SoundChannel::SoundChannel()
: m_pSource(0)
, m_nState(CHANNEL_IDLE)
, m_pBuffer(0)
, m_hFile(0)
, m_bLooping(0)
, m_nWritten(0)
, m_nTotal(0)
, m_nRingSize(0)
, m_nBusy(0)
{
}

// FUNCTION: HYPERX 0x0048fce0
SoundChannel::~SoundChannel()
{
    SoundChannel::Stop();
}

// FUNCTION: HYPERX 0x0048fe10
unsigned int SoundChannel::ReadFromFile(void* pBuffer, unsigned int nBytes)
{
    unsigned int nRead = 0;

    ReadFile(m_hFile, pBuffer, nBytes, (LPDWORD)&nRead, 0);

    return nRead;
}

// A lost buffer is restored once; an exhausted source loops back or pads with silence.
// FUNCTION: HYPERX 0x0048fe40
int SoundChannel::FillBuffer(unsigned int nBytes, unsigned int nOffset)
{
    int   nRetry;
    int   nResult;
    void* apSegments[2];
    DWORD anSegmentBytes[2];

    if (m_pBuffer == 0)
    {
        return 0;
    }

    if (m_pSource == 0)
    {
        return 0;
    }

    char* const pSource = m_pSource->m_pSamples;

    if (pSource == 0 && m_hFile == 0)
    {
        return 0;
    }

    if ((int)nBytes <= 0)
    {
        return 0;
    }

    for (nRetry = 0; nRetry < SOUND_LOCK_RETRIES; nRetry++)
    {
        nResult = m_pBuffer->Lock(nOffset, nBytes, &apSegments[0], &anSegmentBytes[0], &apSegments[1], &anSegmentBytes[1], 0);

        if (nResult == DS_OK)
        {
            break;
        }

        if (nResult != DSERR_BUFFERLOST)
        {
            return 0;
        }

        if (m_pBuffer->Restore() != DS_OK)
        {
            return 0;
        }
    }

    if (nRetry >= SOUND_LOCK_RETRIES)
    {
        return 0;
    }

    for (int nSegment = 0; nSegment < 2; nSegment++)
    {
        const int nRoom    = m_nTotal - m_nWritten;
        char*     pSegment = (char*)apSegments[nSegment];

        if (pSegment == 0)
        {
            break;
        }

        int nTake  = anSegmentBytes[nSegment];
        int nSpill = 0;

        if (nTake > nRoom)
        {
            nSpill = nTake - nRoom;
            nTake  = nRoom;
        }

        if (nTake != 0)
        {
            if (pSource != 0)
            {
                memmove(pSegment, pSource + m_nWritten, nTake);
            }
            else
            {
                const int nRead = ReadFromFile(pSegment, nTake);

                if (nRead != nTake)
                {
                    const int nMissing = nTake - nRead;

                    // Eight-bit samples are unsigned, so their silence is 0x80.
                    memset(pSegment + nRead, (char)((m_format.wBitsPerSample == 8) ? 0x80 : 0x00), nMissing);

                    m_nState = CHANNEL_DRAINED;
                    break;
                }
            }

            m_nWritten += nTake;
        }

        if (m_nWritten < m_nTotal)
        {
            continue;
        }

        pSegment += nTake;

        if (m_bLooping != 0)
        {
            if (nSpill != 0)
            {
                if (pSource != 0)
                {
                    memmove(pSegment, pSource, nSpill);
                    m_nWritten = nSpill;
                }
                else if (SetFilePointer(m_hFile, m_pSource->m_nDataOffset, 0, FILE_BEGIN) != 0xffffffff && ReadFromFile(pSegment, nSpill) == (unsigned int)nSpill)
                {
                    m_nWritten = nSpill;
                }
                else
                {
                    m_bLooping = 0;
                }
            }

            if (m_bLooping != 0)
            {
                continue;
            }
        }

        if (nSpill != 0)
        {
            memset(pSegment, (char)((m_format.wBitsPerSample == 8) ? 0x80 : 0x00), nSpill);
        }

        m_nState = CHANNEL_DRAINED;
    }

    m_pBuffer->Unlock(apSegments[0], anSegmentBytes[0], apSegments[1], anSegmentBytes[1]);

    return 1;
}

#define STREAM_THRESHOLD     0x8800
#define RING_SMALL           0x7800
#define RING_LARGE           0x19000
#define RING_LARGE_THRESHOLD 0x19000

__inline int SoundChannel::IsResidentGroup(const SoundEntry* pEntry, int nGroup)
{
    return pEntry->m_pSamples != 0 && pEntry->m_nGroup == nGroup;
}

// Anything over 0x8800 bytes streams through a ring; the busy count guards the setup.
// FUNCTION: HYPERX 0x00490070
unsigned int SoundChannel::StartPlayback(IDirectSound* pDevice, SoundEntry* pSource, int bLooping)
{
    unsigned int   nResult;
    unsigned int   nDataSize;
    unsigned short nBlockAlign;

    m_nBusy++;

    nResult = 0;

    Stop();

    nDataSize = pSource->m_nDataBytes;

    if (nDataSize == 0)
    {
        goto done;
    }

    memset(&m_desc, 0, sizeof(m_desc));

    memset(&m_format, 0, sizeof(m_format));

    m_format.wf.nChannels      = (WORD)pSource->m_nChannels;
    m_format.wf.nSamplesPerSec = pSource->m_nSamplesPerSec;

    nBlockAlign = (unsigned short)((short)(pSource->m_nBitsPerSample >> 3) * pSource->m_nChannels);

    m_format.wf.nBlockAlign     = nBlockAlign;
    m_format.wf.nAvgBytesPerSec = nBlockAlign * pSource->m_nSamplesPerSec;
    m_format.wf.wFormatTag      = WAVE_FORMAT_PCM;
    m_format.wBitsPerSample     = (WORD)pSource->m_nBitsPerSample;
    m_desc.pFormat              = &m_format;
    m_desc.dwSize               = sizeof(m_desc);
    m_desc.dwFlags              = DSBCAPS_CTRLVOLUME;

    m_nWritten = 0;
    m_nTotal   = nDataSize;

    if (!(IsResidentGroup(pSource, SOUND_GROUP_PRELOAD) || (IsResidentGroup(pSource, SOUND_GROUP_LOOPING) && pSource->m_nIndex == SOUND_SXXAMBA2)) && nDataSize > STREAM_THRESHOLD)
    {
        m_nRingSize = RING_SMALL;

        if (pSource->m_pSamples == 0 && nDataSize >= RING_LARGE_THRESHOLD)
        {
            m_nRingSize = RING_LARGE;
        }

        m_desc.dwBufferBytes = m_nRingSize;
    }
    else
    {
        m_nRingSize          = 0;
        m_desc.dwBufferBytes = pSource->m_nDataBytes;
    }

    if (pDevice->CreateSoundBuffer((LPCDSBUFFERDESC)&m_desc, &m_pBuffer, 0) != DS_OK)
    {
        m_desc.dwFlags = 0;

        if (pDevice->CreateSoundBuffer((LPCDSBUFFERDESC)&m_desc, &m_pBuffer, 0) != DS_OK)
        {
            Stop();

            goto done;
        }
    }

    if (pSource->m_pSamples == 0)
    {
        PathString strPath(MAX_PATH);

        strPath.m_str.FormatResourcePath(g_gameApp.m_strSoundsPath.m_str.m_pchData, pSource->m_pszName, g_pszWaveExtension);

        m_hFile = CreateFileA(strPath.m_str.m_pchData, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, 0);

        if (m_hFile == 0)
        {
            Stop();

            goto done;
        }

        if (pSource->m_nDataOffset != 0 && SetFilePointer(m_hFile, pSource->m_nDataOffset, 0, FILE_BEGIN) == 0xffffffff)
        {
            Stop();

            goto done;
        }
    }

    m_nState   = CHANNEL_PLAYING;
    m_pSource  = pSource;
    m_bLooping = bLooping;

    if (m_nRingSize == 0)
    {
        if (FillBuffer(nDataSize, 0) == 0)
        {
            Stop();

            goto done;
        }

        if (m_pBuffer->Play(0, 0, 0) != DS_OK)
        {
            Stop();

            goto done;
        }
    }
    else
    {
        FillBuffer(m_nRingSize / 2, 0);
        FillBuffer(m_nRingSize / 2, m_nRingSize / 2);

        m_nRingHalf = 0;

        if (m_pBuffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
        {
            Stop();

            goto done;
        }
    }

    nResult = 1;

done:

    m_nBusy--;

    return nResult;
}

// FUNCTION: HYPERX 0x00490330
void SoundChannel::Stop()
{
    m_nBusy++;

    if (m_nState != CHANNEL_IDLE)
    {
        if (m_pBuffer == 0)
        {
            m_nBusy--;

            return;
        }

        m_pBuffer->Stop();

        m_pBuffer->Release();

        m_pBuffer = 0;

        if (m_pSource != 0)
        {
            m_pSource->m_nPlayState = SOUND_STATE_STOPPED;
        }
    }

    if (m_hFile != 0)
    {
        CloseHandle(m_hFile);

        m_hFile = 0;
    }

    m_pSource   = 0;
    m_nState    = CHANNEL_IDLE;
    m_pBuffer   = 0;
    m_hFile     = 0;
    m_bLooping  = 0;
    m_nWritten  = 0;
    m_nTotal    = 0;
    m_nRingSize = 0;

    m_nBusy--;
}

// A re-entrancy guard; a drained channel stops here, which is how a stream ends.
// FUNCTION: HYPERX 0x004903b0
void SoundChannel::StreamUpdate()
{
    m_nBusy++;

    if (m_nBusy != 1)
    {
        m_nBusy--;

        return;
    }

    if (m_pBuffer != 0)
    {
        if (m_nRingSize != 0)
        {
            DWORD nPlay;
            DWORD nWrite;

            if (m_pBuffer->GetCurrentPosition(&nPlay, &nWrite) != DS_OK)
            {
                Stop();

                m_nBusy--;

                return;
            }

            const int nHalf = m_nRingSize / 2;
            int       nFill = m_nRingSize / 2;

            if (nPlay < (unsigned int)nFill)
            {
                nFill = 0;
            }

            if (nFill != m_nRingHalf)
            {
                if (m_nState == CHANNEL_DRAINED)
                {
                    Stop();

                    m_nBusy--;

                    return;
                }

                FillBuffer(nHalf, m_nRingHalf);

                m_nRingHalf = nFill;

                m_nBusy--;

                return;
            }
        }
        else
        {
            DWORD dwStatus = 1;

            if (m_pBuffer->GetStatus(&dwStatus) != DS_OK || (dwStatus & (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING)) == 0)
            {
                Stop();

                m_nBusy--;

                return;
            }
        }
    }

    m_nBusy--;
}
