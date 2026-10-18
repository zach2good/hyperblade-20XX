#include "common/compat.h"
#include "common/path.h"
#include <SDL3/SDL.h>
#include "globals.h"
#include "app/gameapp.h"
#include "audiomgr.h"
#include "soundmixer.h"
#include "soundmanager.h"
#include "engine/gameoptions.h"
#include "common/pathstring.h"
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <new>
#include "sound/wave.h"
#include "sound/soundids.h"
#include <filesystem>
#include <system_error>

// FUNCTION: HYPERX 0x00490830
AudioMgr::AudioMgr()
: nInitTimeMs(0)
, pEntries(nullptr)
, wNoSound(0)
, wNoCD(0)
, nUnused(0)
{
}

// Init allocates the manager with new; it is freed here with delete[].
// FUNCTION: HYPERX 0x00490880
AudioMgr::~AudioMgr()
{
    g_soundMixer.Shutdown();

    if (pEntries != nullptr)
    {
        for (int nIndex = 0; nIndex < pEntries->m_nSoundCount; nIndex++)
        {
            SoundEntry* pEntry = pEntries->m_apSounds[nIndex];

            if (pEntry != nullptr && pEntry->m_pSamples != nullptr)
            {
                free(pEntry->m_pSamples);
            }
        }

        pEntries->FreeEntries();

        delete[] pEntries;
    }
}

// A device that will not open mutes the manager; the mixer lines are set up either way.
// FUNCTION: HYPERX 0x00490990
void AudioMgr::Init()
{
    auto* const pSounds = new SoundManager;

    pEntries = pSounds;

    // STRING: HYPERX 0x004c8a90
    pSounds->Load("SndFiles.lst");

    if (wNoSound == 0)
    {
        if (g_soundMixer.Init() == 0)
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

        LoadSound(pSound);

        nIndex++;
    }

    SfxFadeVolume(static_cast<int>(g_pGameOptions->ReadSfxVolume()));

    CDFadeVolume(static_cast<int>(g_pGameOptions->ReadMusicVolume()));
}

// Only mode one reads the bytes; the others keep the offset for the streamer to come back to.
// FUNCTION: HYPERX 0x00490aa0
int AudioMgr::LoadSound(SoundEntry* pEntry)
{
    if (pEntry->m_nLoadState < 0 || pEntry->m_nLoadState > kSoundLoadLastTry)
    {
        return 0;
    }

    pEntry->m_nSamplesPerSec = 0;
    pEntry->m_nBitsPerSample = 0;
    pEntry->m_nChannels      = 0;
    pEntry->m_nDataBytes     = 0;
    pEntry->m_nDataOffset    = 0;
    pEntry->m_pSamples       = nullptr;
    pEntry->m_nUnusedB       = 0;

    PathString strPath(kMaxPathLength);

    strPath.m_str.FormatResourcePath(g_gameApp.m_strSoundsPath.m_str.m_pchData, pEntry->m_pszName, g_pszWaveExtension);

    auto info = Wave::Info{};

    const auto result = Wave::ReadInfo(strPath.m_str.m_pchData, info);

    if (result == Wave::Result::NO_FILE)
    {
        (pEntry->m_nLoadState)++;

        return 0;
    }

    if (result != Wave::Result::OK)
    {
        pEntry->m_nLoadState = kSoundLoadFailed;

        return 0;
    }

    pEntry->m_nSamplesPerSec = info.nSamplesPerSec;
    pEntry->m_nChannels      = info.nChannels;
    pEntry->m_nBitsPerSample = info.nBitsPerSample;
    pEntry->m_nDataBytes     = info.nDataBytes;
    pEntry->m_nDataOffset    = info.nDataOffset;

    if (pEntry->m_nLoadMode != kSoundLoadWhole)
    {
        return 0;
    }

    pEntry->m_pSamples = static_cast<char*>(malloc(pEntry->m_nDataBytes));

    if (pEntry->m_pSamples == nullptr)
    {
        pEntry->m_nDataBytes  = 0;
        pEntry->m_nDataOffset = 0;

        return 0;
    }

    if (!Wave::ReadSamples(strPath.m_str.m_pchData, pEntry->m_nDataOffset, pEntry->m_nDataBytes, pEntry->m_pSamples))
    {
        free(pEntry->m_pSamples);

        pEntry->m_pSamples    = nullptr;
        pEntry->m_nDataBytes  = 0;
        pEntry->m_nDataOffset = 0;
        pEntry->m_nLoadState  = kSoundLoadFailed;

        return 0;
    }

    return pEntry->m_nDataBytes;
}

// FUNCTION: HYPERX 0x00490c80
void AudioMgr::PlayGroup(int nGroup)
{
    if (wNoSound == 0)
    {
        SoundEntry* const pSound = pEntries->PickFromGroup(nGroup);

        if (pSound != nullptr)
        {
            if (pSound->m_nDataBytes == 0)
            {
                LoadSound(pSound);
            }

            g_soundMixer.Play(pSound);
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

            if (pSound != nullptr)
            {
                if (IsPlaying(nVoice) == 0 || (pSound->m_nGroup != kSoundGroupLooping && pSound->m_pSamples != nullptr))
                {
                    if (pSound->m_nDataBytes == 0)
                    {
                        LoadSound(pSound);
                    }

                    g_soundMixer.Play(pSound);
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

        if (pOwner != nullptr)
        {
            g_soundMixer.StopEntry(pOwner);
        }
    }
}

// FUNCTION: HYPERX 0x00490d60
int AudioMgr::IsPlaying(int nVoice)
{
    if (nVoice < pEntries->m_nSoundCount)
    {
        SoundEntry* const pVoice = pEntries->m_apSounds[nVoice];

        if (pVoice != nullptr)
        {
            if (pVoice->m_nPlayState == kSoundStatePlaying)
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
        g_soundMixer.FadeSfxVolume(nVolume);
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

    return g_soundMixer.ReadSfxVolume();
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

// The CD's audio tracks, ripped to Data/Music/TrackNN.flac, .mp3 or .wav.
constexpr auto kMusicTrackMax = 99;

static const char* const s_apszMusicFormats[] = { "flac", "mp3", "wav" };

static bool MusicPath(int nTrack, char* pszPath, size_t nSize)
{
    for (const char* const pszFormat : s_apszMusicFormats)
    {
        snprintf(pszPath, nSize, "%sMusic\\Track%02d.%s", g_gameApp.m_strExeDir.m_str.m_pchData, nTrack, pszFormat);

        std::error_code error;

        if (std::filesystem::exists(Path::Native(pszPath), error))
        {
            return true;
        }
    }

    return false;
}

static const std::vector<int>& MusicTracks();

bool AudioMgr::HasRippedMusic()
{
    return !MusicTracks().empty();
}

static const std::vector<int>& MusicTracks()
{
    static const auto s_anTracks = []
    {
        std::vector<int> anTracks;
        char             szPath[kMaxPathLength];

        for (int nTrack = 1; nTrack <= kMusicTrackMax; nTrack++)
        {
            if (MusicPath(nTrack, szPath, sizeof(szPath)))
            {
                anTracks.push_back(nTrack);
            }
        }

        return anTracks;
    }();

    return s_anTracks;
}

// A random track is one not in the recent-tracks ring; with no tracks the music stays off.
// FUNCTION: HYPERX 0x00490de0
void AudioMgr::PlayCDTrack(int nTrack)
{
    if (wNoCD != 0)
    {
        return;
    }

    StopCD();

    const auto& anTracks = MusicTracks();
    const int   nTracks  = static_cast<int>(anTracks.size());

    if (nTracks == 0)
    {
        wNoCD = 1;

        return;
    }

    if (nTrack == kCDTrackRandom)
    {
        srand(static_cast<unsigned int>(SDL_GetTicks()));

        int nPicked = kCDTrackRandom;
        int nScan   = g_nCDRecentCursor;

        for (int i = 0; i < nTracks; i++)
        {
            const int nPick = static_cast<int>(static_cast<long long>(rand()) * nTracks / (static_cast<long long>(RAND_MAX) + 1));

            if (g_anCDRecentTracks[nScan] != nPick)
            {
                nPicked = nPick;
                break;
            }

            nScan++;

            if (nScan >= g_nCDRecentTracks)
            {
                nScan = 0;
            }
        }

        if (nPicked == kCDTrackRandom)
        {
            nPicked = static_cast<int>(static_cast<long long>(rand()) * nTracks / (static_cast<long long>(RAND_MAX) + 1));
        }

        g_anCDRecentTracks[g_nCDRecentCursor] = nPicked;
        g_nCDRecentCursor++;

        if (g_nCDRecentCursor >= g_nCDRecentTracks)
        {
            g_nCDRecentCursor = 0;
        }

        nTrack = anTracks[nPicked];
    }

    char szPath[kMaxPathLength];

    if (MusicPath(nTrack, szPath, sizeof(szPath)))
    {
        g_soundMixer.PlayMusic(szPath);
    }
}

// FUNCTION: HYPERX 0x00491070
void AudioMgr::StopCD()
{
    g_soundMixer.StopMusic();
}

// FUNCTION: HYPERX 0x00491090
int AudioMgr::CDReadVolume()
{
    return g_soundMixer.ReadMusicVolume();
}

// FUNCTION: HYPERX 0x004910a0
void AudioMgr::CDFadeVolume(int nVolume)
{
    if (wNoCD == 0)
    {
        g_soundMixer.FadeMusicVolume(nVolume);
    }
}

// Returns how many it loaded; a short read throws the size away so the entry reads unloaded.
// FUNCTION: HYPERX 0x004910f0
int AudioMgr::PreloadGroup(int nGroup)
{
    SoundEntry* pSnd;
    int         nCount;
    int         nLoaded;

    nCount  = pEntries->m_nSoundCount;
    nLoaded = 0;

    for (int nIndex = 0; nIndex < nCount; nIndex++)
    {
        pSnd = pEntries->m_apSounds[nIndex];

        if (pSnd->m_nGroup != nGroup)
        {
            continue;
        }

        if (pSnd->m_nDataBytes == 0)
        {
            if (pSnd->m_nLoadState < 0 || pSnd->m_nLoadState > kSoundLoadLastTry)
            {
                continue;
            }

            LoadSound(pSnd);
        }

        if (pSnd->m_nDataBytes == 0)
        {
            continue;
        }

        if (pSnd->m_pSamples != nullptr)
        {
            nLoaded++;

            continue;
        }

        PathString strPath(kMaxPathLength);

        strPath.m_str.FormatResourcePath(g_gameApp.m_strSoundsPath.m_str.m_pchData, pSnd->m_pszName, g_pszWaveExtension);

        pSnd->m_pSamples = static_cast<char*>(malloc(pSnd->m_nDataBytes));

        if (pSnd->m_pSamples == nullptr || !Wave::ReadSamples(strPath.m_str.m_pchData, pSnd->m_nDataOffset, pSnd->m_nDataBytes, pSnd->m_pSamples))
        {
            free(pSnd->m_pSamples);

            pSnd->m_pSamples   = nullptr;
            pSnd->m_nDataBytes = 0;

            continue;
        }

        nLoaded++;
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
            if (pEntry->m_pSamples != nullptr)
            {
                free(pEntry->m_pSamples);

                pEntry->m_pSamples = nullptr;
            }
        }
    }
}

// FUNCTION: HYPERX 0x00491320
void AudioMgr::StopAllSfx()
{
    g_soundMixer.StopAll();
}
