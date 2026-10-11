#include "common/compat.h"
#include "globals.h"
#include "app/gameapp.h"
#include "audiomgr.h"
#include "mixer.h"
#include "dsoundmgr.h"
#include "soundmanager.h"
#include "engine/gameoptions.h"
#include "common/pathstring.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <new.h>
#include <windows.h>
#include "app/bringup.h"
#include "sound/wave.h"
#include "sound/soundids.h"

// FUNCTION: HYPERX 0x00490830
AudioMgr::AudioMgr()
: nInitTimeMs(0)
, pEntries(0)
, wNoSound(0)
, wNoCD(0)
, hWnd(0)
, nUnused(0)
, wMciDevice(0)
{
}

// Init allocates the manager with new; it is freed here with delete[].
// FUNCTION: HYPERX 0x00490880
AudioMgr::~AudioMgr()
{
    if (g_mixerWaveOut.m_hMixer != 0)
    {
        mixerClose(g_mixerWaveOut.m_hMixer);

        g_mixerWaveOut.m_hMixer = 0;
    }

    if (g_mixerCompactDisc.m_hMixer != 0)
    {
        mixerClose(g_mixerCompactDisc.m_hMixer);

        g_mixerCompactDisc.m_hMixer = 0;
    }

    g_dsoundMgr.Shutdown();

    if (pEntries != 0)
    {
        for (int nIndex = 0; nIndex < pEntries->m_nSoundCount; nIndex++)
        {
            SoundEntry* pEntry = pEntries->m_apSounds[nIndex];

            if (pEntry != 0 && pEntry->m_pSamples != 0)
            {
                free(pEntry->m_pSamples);
            }
        }

        pEntries->FreeEntries();

        if (pEntries != 0)
        {
            delete[] pEntries;
        }
    }

    if (wMciDevice != 0)
    {
        mciSendCommandA(wMciDevice, MCI_STOP, 0, 0);
        mciSendCommandA(wMciDevice, MCI_CLOSE, 0, 0);
    }
}

// A device that will not open mutes the manager; the mixer lines are set up either way.
// FUNCTION: HYPERX 0x00490990
void AudioMgr::Init(HWND hWindow)
{
    hWnd = hWindow;

    SoundManager* const pSounds = new SoundManager;

    int nCost = 0;

    pEntries = pSounds;

    // STRING: HYPERX 0x004c8a90
    pSounds->Load("SndFiles.lst");

    if (wNoSound == 0)
    {
        if (g_dsoundMgr.Init(hWindow) == 0)
        {
            wNoSound = 1;
        }
    }

    int       nIndex = 0;
    const int nCount = pEntries->m_nSoundCount;

    while (nIndex < nCount)
    {
        SoundEntry* const pSound = pEntries->m_apSounds[nIndex];

        pSound->m_nLoadState = 0;

        nCost += LoadSound(pSound);

        nIndex++;
    }

    g_mixerWaveOut.Open(MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT);

    SfxFadeVolume((int)g_pGameOptions->ReadSfxVolume());

    g_mixerCompactDisc.Open(MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC);

    CDFadeVolume((int)g_pGameOptions->ReadMusicVolume());
}

#ifdef HYPERX_BRINGUP
#define LOAD_TRACE_TEXT(w, v) BringUp::TraceText(w, v)
#else
#define LOAD_TRACE_TEXT(w, v) ((void)0)
#endif

// Only mode one reads the bytes; the others keep the offset for the streamer to come back to.
// FUNCTION: HYPERX 0x00490aa0
int AudioMgr::LoadSound(SoundEntry* pEntry)
{
    WAVEFORMATEX* pFormat;
    MMCKINFO      chunk;
    MMCKINFO      parent;
    int           nRead;
    int           nCode;

    int nLoadedBytes = 0;

    if (pEntry->m_nLoadState < 0 || pEntry->m_nLoadState > SOUND_LOAD_LAST_TRY)
    {
        return 0;
    }

    pEntry->m_nSamplesPerSec = 0;
    pEntry->m_nBitsPerSample = 0;
    pEntry->m_nChannels      = 0;
    pEntry->m_nDataBytes     = 0;
    pEntry->m_nDataOffset    = 0;
    pEntry->m_pSamples       = 0;
    pEntry->m_nUnusedB       = 0;

    PathString strPath(MAX_PATH);

    strPath.m_str.FormatResourcePath(g_gameApp.m_strSoundsPath.m_str.m_pchData, pEntry->m_pszName, g_pszWaveExtension);

    HMMIO hmmio = 0;

    if (Wave::Open(strPath.m_str.m_pchData, &hmmio, &pFormat, &parent) != 0)
    {
        // The path as well as the fact: only the path tells a missing wave from a wrong directory.
        LOAD_TRACE_TEXT("loadOpenFailed", strPath.m_str.m_pchData);

        (pEntry->m_nLoadState)++;
    }
    else if (pFormat->wFormatTag != WAVE_FORMAT_PCM)
    {
        pEntry->m_nLoadState = SOUND_LOAD_FAILED;
    }
    else if (pFormat->cbSize != 0)
    {
        pEntry->m_nLoadState = SOUND_LOAD_FAILED;
    }
    else
    {
        pEntry->m_nSamplesPerSec = pFormat->nSamplesPerSec;
        pEntry->m_nChannels      = pFormat->nChannels;
        pEntry->m_nBitsPerSample = pFormat->wBitsPerSample;

        if (Wave::FindDataChunk(&hmmio, &chunk, &parent) != 0)
        {
            pEntry->m_nLoadState = SOUND_LOAD_FAILED;
        }
        else
        {
            pEntry->m_nDataBytes  = chunk.cksize;
            pEntry->m_nDataOffset = chunk.dwDataOffset;

            if (pEntry->m_nLoadMode == SOUND_LOAD_WHOLE)
            {
                pEntry->m_pSamples = (char*)malloc(pEntry->m_nDataBytes);

                if (pEntry->m_pSamples == 0)
                {
                    pEntry->m_nDataBytes  = 0;
                    pEntry->m_nDataOffset = 0;
                }
                else
                {
                    nCode = Wave::ReadChunk(hmmio, chunk.cksize, pEntry->m_pSamples, &chunk, &nRead);

                    if (nCode != 0)
                    {
                        free(pEntry->m_pSamples);

                        pEntry->m_pSamples    = 0;
                        pEntry->m_nDataBytes  = 0;
                        pEntry->m_nDataOffset = 0;
                        pEntry->m_nLoadState  = SOUND_LOAD_FAILED;
                    }
                    else
                    {
                        nLoadedBytes = pEntry->m_nDataBytes;
                    }
                }
            }
        }
    }

    if (hmmio != 0)
    {
        Wave::Close(&hmmio, &pFormat);
    }

    return nLoadedBytes;
}

// FUNCTION: HYPERX 0x00490c80
void AudioMgr::PlayGroup(int nGroup)
{
    if (wNoSound == 0)
    {
        SoundEntry* const pSound = pEntries->PickFromGroup(nGroup);

        if (pSound != 0)
        {
            if (pSound->m_nDataBytes == 0)
            {
                LoadSound(pSound);
            }

            g_dsoundMgr.Play(pSound);
        }
    }
}

// A playing sound restarts only when it does not loop and its samples are resident.
// FUNCTION: HYPERX 0x00490cc0
void AudioMgr::PlaySpecific(int nVoice)
{
    if (wNoSound == 0)
    {
        if (nVoice < pEntries->m_nSoundCount)
        {
            SoundEntry* pSound = pEntries->m_apSounds[nVoice];

            if (pSound != 0)
            {
                if (IsPlaying(nVoice) == 0 || (pSound->m_nGroup != SOUND_GROUP_LOOPING && pSound->m_pSamples != 0))
                {
                    if (pSound->m_nDataBytes == 0)
                    {
                        LoadSound(pSound);
                    }

                    g_dsoundMgr.Play(pSound);
                }
            }
        }
    }
}

// FUNCTION: HYPERX 0x00490d30
void AudioMgr::StopEntry(int nSlot)
{
    if (nSlot < pEntries->m_nSoundCount)
    {
        SoundEntry* const pOwner = pEntries->m_apSounds[nSlot];

        if (pOwner != 0)
        {
            g_dsoundMgr.StopEntry(pOwner);
        }
    }
}

// FUNCTION: HYPERX 0x00490d60
int AudioMgr::IsPlaying(int nVoice)
{
    if (nVoice < pEntries->m_nSoundCount)
    {
        SoundEntry* const pVoice = pEntries->m_apSounds[nVoice];

        if (pVoice != 0)
        {
            if (pVoice->m_nPlayState == SOUND_STATE_PLAYING)
            {
                return 1;
            }
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x00490da0
void AudioMgr::SfxFadeVolume(int nVolume)
{
    if (wNoSound == 0)
    {
        g_dsoundMgr.FadeSfxVolume(nVolume);
    }
}

// A set mute word returns zero without reaching the fader.
// FUNCTION: HYPERX 0x00490dc0
int AudioMgr::SfxReadVolume()
{
    if (wNoSound != 0)
    {
        return 0;
    }

    return g_dsoundMgr.ReadSfxVolume();
}

// Twenty recent tracks, minus one until played; only PlayCDTrack touches it or its cursor.
// GLOBAL: HYPERX 0x004c89ac
int g_anCDRecentTracks[20] = {
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
};

// GLOBAL: HYPERX 0x004c89fc
int g_nCDRecentTracks = 20;

// GLOBAL: HYPERX 0x004d21d8
int g_nCDRecentCursor;

// Opens "cdaudio" lazily; a random track is one not in the recent-tracks ring.
// FUNCTION: HYPERX 0x00490de0
void AudioMgr::PlayCDTrack(int nTrack)
{
    MCI_PLAY_PARMS      mciPlay;
    MCI_STATUS_PARMS    mciStatus;
    MCI_SET_PARMS       mciSet;
    MCI_WAVE_OPEN_PARMS mciOpen;
    char                szError[MAX_PATH];
    MCIERROR            err;

    if (wNoCD == 0)
    {
        StopCD();

        if (wMciDevice == 0)
        {
            // STRING: HYPERX 0x004c8aa0
            mciOpen.lpstrDeviceType = "cdaudio";

            err = mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE, (DWORD)&mciOpen);

            if (err != 0)
            {
                mciGetErrorStringA(err, szError, sizeof(szError));
                wNoCD = 1;
                return;
            }

            wMciDevice = (unsigned short)mciOpen.wDeviceID;

            mciSet.dwTimeFormat = MCI_FORMAT_TMSF;

            err = mciSendCommandA(wMciDevice, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&mciSet);

            if (err != 0)
            {
                mciGetErrorStringA(err, szError, sizeof(szError));
                mciSendCommandA(wMciDevice, MCI_CLOSE, 0, 0);
                wMciDevice = 0;
                wNoCD      = 1;
                return;
            }
        }

        mciStatus.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;

        err = mciSendCommandA(wMciDevice, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&mciStatus);

        if (err != 0)
        {
            mciGetErrorStringA(err, szError, sizeof(szError));
            mciSendCommandA(wMciDevice, MCI_CLOSE, 0, 0);
            wMciDevice = 0;
            return;
        }

        const int nTracks = (int)mciStatus.dwReturn;

        if (nTrack == CD_TRACK_RANDOM)
        {
            if (nTracks <= 1)
            {
                nTrack = 1;
            }
            else
            {
                int nScan = g_nCDRecentCursor;

                if (nTracks > 0)
                {
                    for (int i = 0; i < nTracks; i++)
                    {
                        const DWORD dwSeed = timeGetTime();
                        srand(dwSeed);

                        const int nPick = rand() * nTracks / (RAND_MAX + 1);

                        if (g_anCDRecentTracks[nScan] != nPick)
                        {
                            nTrack = nPick;
                            break;
                        }

                        nScan++;

                        if (nScan >= g_nCDRecentTracks)
                        {
                            nScan = 0;
                        }
                    }
                }

                if (nTrack == CD_TRACK_RANDOM)
                {
                    nTrack = rand() * nTracks / (RAND_MAX + 1);
                }

                g_anCDRecentTracks[g_nCDRecentCursor] = nTrack;
                g_nCDRecentCursor++;

                if (g_nCDRecentCursor >= g_nCDRecentTracks)
                {
                    g_nCDRecentCursor = 0;
                }
            }

            nTrack++;
        }

        if (wMciDevice != 0)
        {
            unsigned int nFlags = MCI_NOTIFY | MCI_FROM;

            if (nTrack != nTracks)
            {
                nFlags       = MCI_NOTIFY | MCI_FROM | MCI_TO;
                mciPlay.dwTo = MCI_MAKE_TMSF(nTrack + 1, 0, 0, 0);
            }
            else
            {
                mciPlay.dwTo = 0;
            }

            mciPlay.dwFrom     = MCI_MAKE_TMSF(nTrack, 0, 0, 0);
            mciPlay.dwCallback = (DWORD)g_gameApp.m_hWnd;

            err = mciSendCommandA(wMciDevice, MCI_PLAY, nFlags, (DWORD)&mciPlay);

            if (err != 0)
            {
                mciGetErrorStringA(err, szError, sizeof(szError));
                mciSendCommandA(wMciDevice, MCI_CLOSE, 0, 0);
                wMciDevice = 0;
            }
        }
    }
}

// FUNCTION: HYPERX 0x00491070
void AudioMgr::StopCD()
{
    if (wMciDevice != 0)
    {
        mciSendCommandA(wMciDevice, MCI_STOP, 0, 0);
    }
}

// FUNCTION: HYPERX 0x00491090
int AudioMgr::CDReadVolume()
{
    return g_mixerCompactDisc.ReadVolume();
}

// FUNCTION: HYPERX 0x004910a0
void AudioMgr::CDFadeVolume(int nVolume)
{
    if (wNoCD != 0)
    {
        return;
    }

    int nStep = nVolume;

    if (nVolume < 0)
    {
        nStep = 0;
    }
    else if (nVolume >= MIXER_VOLUME_LEVELS)
    {
        nStep = MIXER_VOLUME_LEVELS - 1;
    }

    if (wMciDevice == 0)
    {
        return;
    }

    if (g_mixerCompactDisc.FadeVolume(nStep) != 0)
    {
        g_pGameOptions->WriteMusicVolume(nStep);
    }
}

// Returns how many it loaded; a short read throws the size away so the entry reads unloaded.
// FUNCTION: HYPERX 0x004910f0
int AudioMgr::PreloadGroup(int nGroup)
{
    SoundEntry* pSnd;
    HANDLE      hFile;
    DWORD       nRead;
    int         nCount;
    int         nLoaded;
    int         nBytes;

    nCount  = pEntries->m_nSoundCount;
    nLoaded = 0;
    nBytes  = 0;

    for (int nIndex = 0; nIndex < nCount; nIndex++)
    {
        pSnd = pEntries->m_apSounds[nIndex];

        if (pSnd->m_nGroup != nGroup)
        {
            continue;
        }

        if (pSnd->m_nDataBytes == 0)
        {
            if (pSnd->m_nLoadState < 0 || pSnd->m_nLoadState > SOUND_LOAD_LAST_TRY)
            {
                continue;
            }

            LoadSound(pSnd);
        }

        if (pSnd->m_nDataBytes == 0)
        {
            continue;
        }

        if (pSnd->m_pSamples != 0)
        {
            nBytes += pSnd->m_nDataBytes;
            nLoaded++;

            continue;
        }

        PathString strPath(MAX_PATH);

        strPath.m_str.FormatResourcePath(g_gameApp.m_strSoundsPath.m_str.m_pchData,
                                         pSnd->m_pszName,
                                         g_pszWaveExtension);

        hFile = CreateFileA(strPath.m_str.m_pchData, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);

        if (hFile != 0)
        {
            if (pSnd->m_nDataOffset != 0 && SetFilePointer(hFile, pSnd->m_nDataOffset, 0, FILE_BEGIN) == 0xffffffff)
            {
                CloseHandle(hFile);
            }
            else
            {
                pSnd->m_pSamples = (char*)malloc(pSnd->m_nDataBytes);

                if (pSnd->m_pSamples != 0)
                {
                    nRead = 0;

                    if (ReadFile(hFile, pSnd->m_pSamples, pSnd->m_nDataBytes, &nRead, 0) != 0 && (DWORD)pSnd->m_nDataBytes == nRead)
                    {
                        nBytes += pSnd->m_nDataBytes;
                        nLoaded++;

                        CloseHandle(hFile);
                    }
                    else
                    {
                        pSnd->m_nDataBytes = 0;

                        CloseHandle(hFile);
                    }
                }
                else
                {
                    pSnd->m_nDataBytes = 0;

                    CloseHandle(hFile);
                }
            }
        }
    }

    return nLoaded;
}

// The buffer pointer is cleared after the free, so freeing a group twice is harmless.
// FUNCTION: HYPERX 0x004912d0
void AudioMgr::FreeGroupBuffers(int nGroup)
{
    const int nEntries = pEntries->m_nSoundCount;

    for (int iEntry = 0; iEntry < nEntries; iEntry++)
    {
        SoundEntry* const pEntry = pEntries->m_apSounds[iEntry];

        if (pEntry->m_nGroup == nGroup)
        {
            if (pEntry->m_pSamples != 0)
            {
                free(pEntry->m_pSamples);

                pEntry->m_pSamples = 0;
            }
        }
    }
}

// FUNCTION: HYPERX 0x00491320
void AudioMgr::StopAllSfx()
{
    g_dsoundMgr.StopAll();
}
