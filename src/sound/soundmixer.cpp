#include "common/compat.h"
#include "soundmixer.h"
#include "engine/gameoptions.h"
#include "globals.h"
#include "mixer.h"
#include "soundmanager.h"
#include "app/gameapp.h"
#include "common/pathstring.h"
#include "sound/soundids.h"
#include <SDL3/SDL.h>
#include <miniaudio.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

SoundMixer g_soundMixer;

constexpr auto MIX_CHANNELS = 2;
constexpr auto MIX_RATE     = 44100;
constexpr auto MIX_FRAMES   = 1024;

// The original streamed anything this large, and only a streamed sound ever looped.
constexpr auto STREAM_THRESHOLD = 0x8800u;

namespace
{

struct Voice
{
    SoundEntry*                pSource = nullptr;
    std::vector<unsigned char> samples;
    ma_audio_buffer_ref        buffer{};
    ma_sound                   sound{};
};

int IsResidentGroup(const SoundEntry* pEntry, int nGroup)
{
    return pEntry->m_pSamples != 0 && pEntry->m_nGroup == nGroup;
}

bool Streams(const SoundEntry* pEntry)
{
    if (IsResidentGroup(pEntry, SOUND_GROUP_PRELOAD))
    {
        return false;
    }

    if (IsResidentGroup(pEntry, SOUND_GROUP_LOOPING) && pEntry->m_nIndex == SOUND_SXXAMBA2)
    {
        return false;
    }

    return pEntry->m_nDataBytes > STREAM_THRESHOLD;
}

bool ReadData(const SoundEntry* pEntry, std::vector<unsigned char>& samples)
{
    PathString strPath(MAX_PATH);

    strPath.m_str.FormatResourcePath(g_gameApp.m_strSoundsPath.m_str.m_pchData, pEntry->m_pszName, g_pszWaveExtension);

    std::FILE* const pFile = std::fopen(strPath.m_str.m_pchData, "rb");

    if (pFile == nullptr)
    {
        return false;
    }

    samples.resize(pEntry->m_nDataBytes);

    const auto bRead = std::fseek(pFile, (long)pEntry->m_nDataOffset, SEEK_SET) == 0 && std::fread(samples.data(), 1, samples.size(), pFile) == samples.size();

    std::fclose(pFile);

    return bRead;
}

void SDLCALL FeedStream(void* pUser, SDL_AudioStream* pStream, int nAdditional, int)
{
    auto* const pEngine = static_cast<ma_engine*>(pUser);

    float afFrames[MIX_FRAMES * MIX_CHANNELS];

    constexpr auto nFrameBytes = (int)sizeof(float) * MIX_CHANNELS;

    while (nAdditional > 0)
    {
        const auto nFrames = std::clamp(nAdditional / nFrameBytes, 1, MIX_FRAMES);

        ma_engine_read_pcm_frames(pEngine, afFrames, (ma_uint64)nFrames, nullptr);

        SDL_PutAudioStreamData(pStream, afFrames, nFrames * nFrameBytes);

        nAdditional -= nFrames * nFrameBytes;
    }
}

} // namespace

// SDL calls FeedStream with the stream locked, so holding the lock keeps the voices still.
struct SoundMixer::Impl
{
    ma_engine                              engine{};
    SDL_AudioStream*                       pStream = nullptr;
    std::array<Voice, SOUND_CHANNEL_COUNT> aVoices;

    void Stop(Voice& voice)
    {
        if (voice.pSource == nullptr)
        {
            return;
        }

        ma_sound_uninit(&voice.sound);
        ma_audio_buffer_ref_uninit(&voice.buffer);

        voice.pSource->m_nPlayState = SOUND_STATE_STOPPED;
        voice.pSource               = nullptr;

        voice.samples.clear();
    }
};

class StreamLock
{
public:
    explicit StreamLock(SDL_AudioStream* pStream)
    : m_pStream(pStream)
    {
        SDL_LockAudioStream(m_pStream);
    }

    ~StreamLock()
    {
        SDL_UnlockAudioStream(m_pStream);
    }

    StreamLock(const StreamLock&)            = delete;
    StreamLock& operator=(const StreamLock&) = delete;

private:
    SDL_AudioStream* m_pStream;
};

// FUNCTION: HYPERX 0x0048fc70
SoundMixer::SoundMixer() = default;

// FUNCTION: HYPERX 0x0048fd10
SoundMixer::~SoundMixer()
{
    Shutdown();
}

// FUNCTION: HYPERX 0x00490520
int SoundMixer::Init()
{
    if (m_pImpl)
    {
        return 1;
    }

    auto pImpl = std::make_unique<Impl>();

    auto config       = ma_engine_config_init();
    config.noDevice   = MA_TRUE;
    config.channels   = MIX_CHANNELS;
    config.sampleRate = MIX_RATE;

    if (ma_engine_init(&config, &pImpl->engine) != MA_SUCCESS)
    {
        // STRING: HYPERX 0x004c8a58
        g_gameApp.Error("Direct Sound Creation Failed.  Sound will be turned off");

        return 0;
    }

    const auto spec = SDL_AudioSpec{ SDL_AUDIO_F32, MIX_CHANNELS, MIX_RATE };

    if (SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        pImpl->pStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedStream, &pImpl->engine);
    }

    if (pImpl->pStream == nullptr)
    {
        ma_engine_uninit(&pImpl->engine);

        // STRING: HYPERX 0x004c8a58
        g_gameApp.Error("Direct Sound Creation Failed.  Sound will be turned off");

        return 0;
    }

    m_pImpl = std::move(pImpl);

    ma_engine_set_volume(&m_pImpl->engine, (float)m_nVolumeStep / (MIXER_VOLUME_LEVELS - 1));

    SDL_ResumeAudioStreamDevice(m_pImpl->pStream);

    return 1;
}

// FUNCTION: HYPERX 0x004906d0
void SoundMixer::Shutdown()
{
    if (!m_pImpl)
    {
        return;
    }

    SDL_DestroyAudioStream(m_pImpl->pStream);

    for (auto& voice : m_pImpl->aVoices)
    {
        m_pImpl->Stop(voice);
    }

    ma_engine_uninit(&m_pImpl->engine);

    m_pImpl.reset();

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// The start time is stamped only when playback succeeded.
// FUNCTION: HYPERX 0x00490780
void SoundMixer::Play(SoundEntry* pSound)
{
    if (!m_pImpl || pSound->m_nDataBytes == 0)
    {
        return;
    }

    const auto format = [&]
    {
        if (pSound->m_nBitsPerSample == 8)
        {
            return ma_format_u8;
        }

        if (pSound->m_nBitsPerSample == 16)
        {
            return ma_format_s16;
        }

        return ma_format_unknown;
    }();

    const auto nFrameBytes = (pSound->m_nBitsPerSample / 8) * pSound->m_nChannels;

    if (format == ma_format_unknown || nFrameBytes == 0)
    {
        return;
    }

    const auto lock = StreamLock(m_pImpl->pStream);

    const auto it = std::find_if(m_pImpl->aVoices.begin(), m_pImpl->aVoices.end(), [](const Voice& voice)
                                 {
                                     return voice.pSource == nullptr;
                                 });

    if (it == m_pImpl->aVoices.end())
    {
        return;
    }

    auto& voice = *it;

    const auto* pData = static_cast<const void*>(pSound->m_pSamples);

    if (pData == nullptr)
    {
        if (!ReadData(pSound, voice.samples))
        {
            voice.samples.clear();

            return;
        }

        pData = voice.samples.data();
    }

    ma_audio_buffer_ref_init(format, pSound->m_nChannels, pData, pSound->m_nDataBytes / nFrameBytes, &voice.buffer);
    voice.buffer.sampleRate = pSound->m_nSamplesPerSec;

    if (ma_sound_init_from_data_source(&m_pImpl->engine, &voice.buffer, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH, nullptr, &voice.sound) != MA_SUCCESS)
    {
        ma_audio_buffer_ref_uninit(&voice.buffer);
        voice.samples.clear();

        return;
    }

    // A looping sound held whole played once in the original, so only a streamed one loops.
    ma_sound_set_looping(&voice.sound, pSound->m_nGroup == SOUND_GROUP_LOOPING && Streams(pSound));
    ma_sound_start(&voice.sound);

    voice.pSource = pSound;

    pSound->m_nPlayState = SOUND_STATE_PLAYING;
    pSound->m_nStartedMs = timeGetTime();
}

// FUNCTION: HYPERX 0x004907d0
void SoundMixer::StopEntry(SoundEntry* pOwner)
{
    if (!m_pImpl)
    {
        return;
    }

    const auto lock = StreamLock(m_pImpl->pStream);

    for (auto& voice : m_pImpl->aVoices)
    {
        if (voice.pSource == pOwner)
        {
            m_pImpl->Stop(voice);
        }
    }
}

// FUNCTION: HYPERX 0x00490800
void SoundMixer::StopAll()
{
    if (!m_pImpl)
    {
        return;
    }

    const auto lock = StreamLock(m_pImpl->pStream);

    for (auto& voice : m_pImpl->aVoices)
    {
        m_pImpl->Stop(voice);
    }
}

// FUNCTION: HYPERX 0x00490490
void SoundMixer::Update()
{
    if (!m_pImpl)
    {
        return;
    }

    const auto lock = StreamLock(m_pImpl->pStream);

    for (auto& voice : m_pImpl->aVoices)
    {
        if (voice.pSource != nullptr && ma_sound_at_end(&voice.sound))
        {
            m_pImpl->Stop(voice);
        }
    }
}

// FUNCTION: HYPERX 0x004904c0
int SoundMixer::ReadSfxVolume() const
{
    return m_nVolumeStep;
}

// FUNCTION: HYPERX 0x004904d0
int SoundMixer::FadeSfxVolume(int nStep)
{
    m_nVolumeStep = std::clamp(nStep, 0, MIXER_VOLUME_LEVELS - 1);

    if (m_pImpl)
    {
        ma_engine_set_volume(&m_pImpl->engine, (float)m_nVolumeStep / (MIXER_VOLUME_LEVELS - 1));
    }

    g_pGameOptions->WriteSfxVolume(m_nVolumeStep);

    return 1;
}
