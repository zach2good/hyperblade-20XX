// The game's eight sound channels, mixed by miniaudio and played through SDL.

#ifndef HYPERBLADE_SOUNDMIXER_H
#define HYPERBLADE_SOUNDMIXER_H
#include "common/compat.h"

#include <memory>

#define SOUND_CHANNEL_COUNT 8

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

    // Marks the sounds that have played to their end as stopped.
    void Update();

private:
    struct Impl;

    std::unique_ptr<Impl> m_pImpl;
    int                   m_nVolumeStep = 0;
};

extern SoundMixer g_soundMixer;

#endif
