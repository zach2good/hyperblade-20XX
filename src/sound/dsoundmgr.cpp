#include "common/compat.h"
#include "dsoundmgr.h"
#include "engine/gameoptions.h"
#include "globals.h"
#include "mixer.h"
#include "soundmanager.h"
#include "soundchannel.h"
#include "soundglobals.h"
#include "app/gameapp.h"
#include "sound/soundids.h"

DSoundMgr g_dsoundMgr;

// The streaming timer's period, in milliseconds.
#define DSOUND_TIMER_MS 0x36

// FUNCTION: HYPERX 0x0048fc70
DSoundMgr::DSoundMgr()
: m_pDevice(0)
, m_pPrimary(0)
, m_nTimerId(0)
, m_hWnd(0)
, m_nUnused(-1)
{
}

// FUNCTION: HYPERX 0x0048fd10
DSoundMgr::~DSoundMgr()
{
    Shutdown();
}

// FUNCTION: HYPERX 0x00490490
void DSoundMgr::Update()
{
    for (int nIndex = 0; nIndex < DSOUND_CHANNEL_COUNT; nIndex++)
    {
        if (m_aChannels[nIndex].m_nState != CHANNEL_IDLE)
        {
            m_aChannels[nIndex].StreamUpdate();
        }
    }
}

// FUNCTION: HYPERX 0x004904c0
int DSoundMgr::ReadSfxVolume()
{
    return g_mixerWaveOut.ReadVolume();
}

// The options are written only when the fade took.
// FUNCTION: HYPERX 0x004904d0
int DSoundMgr::FadeSfxVolume(int nStep)
{
    if (nStep < 0)
    {
        nStep = 0;
    }
    else if (nStep >= MIXER_VOLUME_LEVELS)
    {
        nStep = MIXER_VOLUME_LEVELS - 1;
    }

    if (g_mixerWaveOut.FadeVolume(nStep) != 0)
    {
        g_pGameOptions->WriteSfxVolume(nStep);

        return 1;
    }

    return 0;
}

// The original's descriptor is 36 bytes where this SDK's is 20; DirectSound takes either.
// FUNCTION: HYPERX 0x00490520
int DSoundMgr::Init(HWND hWindow)
{
    SoundBufferDesc desc;

    m_hWnd = hWindow;

    if (m_pDevice != 0)
    {
        return 1;
    }

    try
    {
        m_nTimerId = SetTimer(0, 0, DSOUND_TIMER_MS, DSoundMgr::TimerProc);

        if (m_nTimerId == 0)
        {
            // STRING: HYPERX 0x004c8a58
            throw "Direct Sound Creation Failed.  Sound will be turned off";
        }

        if (DirectSoundCreate(0, &m_pDevice, 0) != DS_OK)
        {
            // STRING: HYPERX 0x004c8a58
            throw "Direct Sound Creation Failed.  Sound will be turned off";
        }

        if (m_pDevice->SetCooperativeLevel(m_hWnd, DSSCL_NORMAL) != DS_OK)
        {
            // STRING: HYPERX 0x004c8a2c
            throw "Direct Sound Set Cooperative Level Failed";
        }

        memset(&desc, 0, sizeof(desc));

        desc.dwSize  = sizeof(desc);
        desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

        if (m_pDevice->CreateSoundBuffer((LPCDSBUFFERDESC)&desc, &m_pPrimary, 0) != DS_OK)
        {
            desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

            if (m_pDevice->CreateSoundBuffer((LPCDSBUFFERDESC)&desc, &m_pPrimary, 0) != DS_OK)
            {
                // STRING: HYPERX 0x004c8a00
                throw "Cannot create primary Direct Sound buffer";
            }
        }

        return 1;
    }
    catch (char* pszMessage)
    {
        g_gameApp.Error(pszMessage);

        Shutdown();
    }

    return 0;
}

// A counter, not a flag, guards re-entry: a nested tick is dropped and the count comes back.
// FUNCTION: HYPERX 0x004906a0
void __stdcall DSoundMgr::TimerProc(HWND, UINT, UINT, DWORD)
{
    g_nSoundTimerDepth++;

    if (g_nSoundTimerDepth == 1)
    {
        g_dsoundMgr.Update();
    }

    g_nSoundTimerDepth--;
}

// FUNCTION: HYPERX 0x004906d0
void DSoundMgr::Shutdown()
{
    if (m_nTimerId != 0)
    {
        if (m_hWnd != 0)
        {
            KillTimer(m_hWnd, m_nTimerId);
        }

        m_nTimerId = 0;
    }

    for (int nIndex = 0; nIndex < DSOUND_CHANNEL_COUNT; nIndex++)
    {
        m_aChannels[nIndex].Stop();
    }

    if (m_pPrimary != 0)
    {
        m_pPrimary->Stop();

        m_pPrimary->Release();

        m_pPrimary = 0;
    }

    if (m_pDevice != 0)
    {
        m_pDevice->Release();

        m_pDevice = 0;
    }
}

// FUNCTION: HYPERX 0x00490750
SoundChannel* DSoundMgr::FindFreeChannel()
{
    SoundChannel* pFound = 0;

    for (int iChannel = 0; iChannel < DSOUND_CHANNEL_COUNT; iChannel++)
    {
        if (m_aChannels[iChannel].m_nState == CHANNEL_IDLE && m_aChannels[iChannel].m_nBusy == 0)
        {
            pFound = &m_aChannels[iChannel];
            break;
        }
    }

    return pFound;
}

// The start time is stamped only when playback succeeded.
// FUNCTION: HYPERX 0x00490780
void DSoundMgr::Play(SoundEntry* pSound)
{
    SoundChannel* const pChannel = FindFreeChannel();

    if (pChannel != 0)
    {
        if (pChannel->StartPlayback(m_pDevice, pSound, pSound->m_nGroup == SOUND_GROUP_LOOPING) != 0)
        {
            pSound->m_nPlayState = SOUND_STATE_PLAYING;

            pSound->m_nStartedMs = timeGetTime();
        }
    }
}

// FUNCTION: HYPERX 0x004907d0
void DSoundMgr::StopEntry(SoundEntry* pOwner)
{
    for (int nIndex = 0; nIndex < DSOUND_CHANNEL_COUNT; nIndex++)
    {
        if (m_aChannels[nIndex].m_nState != CHANNEL_IDLE && m_aChannels[nIndex].m_pSource == pOwner)
        {
            m_aChannels[nIndex].Stop();
        }
    }
}

// FUNCTION: HYPERX 0x00490800
void DSoundMgr::StopAll()
{
    for (int nIndex = 0; nIndex < DSOUND_CHANNEL_COUNT; nIndex++)
    {
        if (m_aChannels[nIndex].m_nState != CHANNEL_IDLE)
        {
            m_aChannels[nIndex].Stop();
        }
    }
}
