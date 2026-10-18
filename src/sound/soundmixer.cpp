#include "common/compat.h"
#include "common/path.h"
#include "soundmixer.h"
#include "engine/gameoptions.h"
#include "globals.h"
#include "soundmanager.h"
#include "wave.h"
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
    return pEntry->m_pSamples != nullptr && pEntry->m_nGroup == nGroup;
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

    return static_cast<unsigned int>(pEntry->m_nDataBytes) > STREAM_THRESHOLD;
}

bool ReadData(const SoundEntry* pEntry, std::vector<unsigned char>& samples)
{
    PathString strPath(MAX_PATH_LENGTH);

    strPath.m_str.FormatResourcePath(g_gameApp.m_strSoundsPath.m_str.m_pchData, pEntry->m_pszName, g_pszWaveExtension);

    samples.resize(pEntry->m_nDataBytes);

    return Wave::ReadSamples(strPath.m_str.m_pchData, pEntry->m_nDataOffset, pEntry->m_nDataBytes, samples.data());
}

void SDLCALL FeedStream(void* pUser, SDL_AudioStream* pStream, int nAdditional, int)
{
    auto* const pEngine = static_cast<ma_engine*>(pUser);

    float afFrames[MIX_FRAMES * MIX_CHANNELS];

    constexpr auto nFrameBytes = static_cast<int>(sizeof(float)) * MIX_CHANNELS;

    while (nAdditional > 0)
    {
        const auto nFrames = std::clamp(nAdditional / nFrameBytes, 1, MIX_FRAMES);

        ma_engine_read_pcm_frames(pEngine, afFrames, static_cast<ma_uint64>(nFrames), nullptr);

        SDL_PutAudioStreamData(pStream, afFrames, nFrames * nFrameBytes);

        nAdditional -= nFrames * nFrameBytes;
    }
}

} // namespace

// SDL calls FeedStream with the stream locked, so holding the lock keeps the voices still.
struct SoundMixer::Impl
{
    ma_engine                              engine{};
    ma_sound_group                         sfx{};
    ma_sound_group                         music{};
    SDL_AudioStream*                       pStream = nullptr;
    std::array<Voice, SOUND_CHANNEL_COUNT> aVoices;
    ma_decoder                             decoder{};
    ma_sound                               track{};
    bool                                   bTrack = false;

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

    void StopTrack()
    {
        if (!bTrack)
        {
            return;
        }

        ma_sound_uninit(&track);
        ma_decoder_uninit(&decoder);

        bTrack = false;
    }
};

float VolumeOf(int nStep)
{
    return static_cast<float>(nStep) / (MIXER_VOLUME_LEVELS - 1);
}

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

    ma_sound_group_init(&pImpl->engine, 0, nullptr, &pImpl->sfx);
    ma_sound_group_init(&pImpl->engine, 0, nullptr, &pImpl->music);

    const auto spec = SDL_AudioSpec{ SDL_AUDIO_F32, MIX_CHANNELS, MIX_RATE };

    if (SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        pImpl->pStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedStream, &pImpl->engine);
    }

    if (pImpl->pStream == nullptr)
    {
        ma_sound_group_uninit(&pImpl->music);
        ma_sound_group_uninit(&pImpl->sfx);
        ma_engine_uninit(&pImpl->engine);

        // STRING: HYPERX 0x004c8a58
        g_gameApp.Error("Direct Sound Creation Failed.  Sound will be turned off");

        return 0;
    }

    m_pImpl = std::move(pImpl);

    ma_sound_group_set_volume(&m_pImpl->sfx, VolumeOf(m_nVolumeStep));
    ma_sound_group_set_volume(&m_pImpl->music, VolumeOf(m_nMusicStep));

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

    m_pImpl->StopTrack();

    ma_sound_group_uninit(&m_pImpl->music);
    ma_sound_group_uninit(&m_pImpl->sfx);
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

    // The voice keeps its own copy, as a DirectSound buffer did: the game frees a group's samples
    // while its sounds may still be playing.
    if (pSound->m_pSamples != nullptr)
    {
        voice.samples.assign(pSound->m_pSamples, pSound->m_pSamples + pSound->m_nDataBytes);
    }
    else if (!ReadData(pSound, voice.samples))
    {
        voice.samples.clear();

        return;
    }

    ma_audio_buffer_ref_init(format, pSound->m_nChannels, voice.samples.data(), pSound->m_nDataBytes / nFrameBytes, &voice.buffer);
    voice.buffer.sampleRate = pSound->m_nSamplesPerSec;

    if (ma_sound_init_from_data_source(&m_pImpl->engine, &voice.buffer, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH, &m_pImpl->sfx, &voice.sound) != MA_SUCCESS)
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
    pSound->m_nStartedMs = static_cast<unsigned int>(SDL_GetTicks());
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
bool SoundMixer::Update()
{
    if (!m_pImpl)
    {
        return false;
    }

    const auto lock = StreamLock(m_pImpl->pStream);

    for (auto& voice : m_pImpl->aVoices)
    {
        if (voice.pSource != nullptr && ma_sound_at_end(&voice.sound))
        {
            m_pImpl->Stop(voice);
        }
    }

    if (m_pImpl->bTrack && ma_sound_at_end(&m_pImpl->track))
    {
        m_pImpl->StopTrack();

        return true;
    }

    return false;
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
        ma_sound_group_set_volume(&m_pImpl->sfx, VolumeOf(m_nVolumeStep));
    }

    g_pGameOptions->WriteSfxVolume(m_nVolumeStep);

    return 1;
}

bool SoundMixer::PlayMusic(const char* pszPath)
{
    if (!m_pImpl)
    {
        return false;
    }

    const auto lock = StreamLock(m_pImpl->pStream);

    m_pImpl->StopTrack();

    const auto config = ma_decoder_config_init(ma_format_f32, MIX_CHANNELS, MIX_RATE);

    if (ma_decoder_init_file(Path::Native(pszPath).c_str(), &config, &m_pImpl->decoder) != MA_SUCCESS)
    {
        return false;
    }

    if (ma_sound_init_from_data_source(&m_pImpl->engine, &m_pImpl->decoder, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH, &m_pImpl->music, &m_pImpl->track) != MA_SUCCESS)
    {
        ma_decoder_uninit(&m_pImpl->decoder);

        return false;
    }

    ma_sound_start(&m_pImpl->track);

    m_pImpl->bTrack = true;

    return true;
}

void SoundMixer::StopMusic()
{
    if (!m_pImpl)
    {
        return;
    }

    const auto lock = StreamLock(m_pImpl->pStream);

    m_pImpl->StopTrack();
}

int SoundMixer::ReadMusicVolume() const
{
    return m_nMusicStep;
}

void SoundMixer::FadeMusicVolume(int nStep)
{
    m_nMusicStep = std::clamp(nStep, 0, MIXER_VOLUME_LEVELS - 1);

    if (m_pImpl)
    {
        ma_sound_group_set_volume(&m_pImpl->music, VolumeOf(m_nMusicStep));
    }

    g_pGameOptions->WriteMusicVolume(m_nMusicStep);
}
