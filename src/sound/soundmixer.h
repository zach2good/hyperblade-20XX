// The game's eight sound channels, mixed by miniaudio and played through SDL.

#ifndef HYPERBLADE_SOUNDMIXER_H
#define HYPERBLADE_SOUNDMIXER_H
#include "common/compat.h"

#include <memory>

constexpr auto kSoundChannelCount = 8;

// The volume options run 0 to 7.
constexpr auto kMixerVolumeLevels = 8;

class SoundMixer
{
public:
    SoundMixer();
    ~SoundMixer();

    SoundMixer(const SoundMixer&)            = delete;
    SoundMixer& operator=(const SoundMixer&) = delete;
    SoundMixer(SoundMixer&&)                 = delete;
    SoundMixer& operator=(SoundMixer&&)      = delete;

    // One when there is a device and zero when not.
    int  Init();
    void Shutdown();

    // A sound with no free channel is dropped without complaint.
    void Play(struct SoundEntry* pSound);
    void StopEntry(struct SoundEntry* pOwner);
    void StopAll();

    int ReadSfxVolume() const;
    int FadeSfxVolume(int nStep);

    // One music track streams from a file at a time.
    bool PlayMusic(const char* pszPath);
    void StopMusic();
    int  ReadMusicVolume() const;
    void FadeMusicVolume(int nStep);

    // Marks the sounds that have played to their end as stopped; true when the music track has.
    bool Update();

    // Public so the audio callback can name it.
    struct Impl;

private:
    std::unique_ptr<Impl> m_pImpl;
    int                   m_nVolumeStep = 0;
    int                   m_nMusicStep  = 0;
};

extern SoundMixer g_soundMixer;

#endif
