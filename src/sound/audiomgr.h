#ifndef HYPERBLADE_AUDIOMGR_H
#define HYPERBLADE_AUDIOMGR_H

#include "sound/audiomgrbase.h"
#include "sound/soundmanager.h"
#include <memory>

// PlayCDTrack's tracks.
constexpr auto kCDTrackRandom = (-1);
constexpr auto kCDTrackMenu   = 10;

// VTABLE: HYPERX 0x004b4080
struct AudioMgr : public AudioMgrBase
{
    void StopAllSfx();

    AudioMgr();

    ~AudioMgr() override;

    AudioMgr(const AudioMgr&)            = delete;
    AudioMgr& operator=(const AudioMgr&) = delete;
    AudioMgr(AudioMgr&&)                 = delete;
    AudioMgr& operator=(AudioMgr&&)      = delete;

    void StopCD() override;

    // Whether any of the CD's tracks are ripped to Data/Music.
    static bool HasRippedMusic();

    int  SfxReadVolume();
    void SfxFadeVolume(int nVolume);
    int  CDReadVolume();
    void CDFadeVolume(int nVolume);

    int  PreloadGroup(int nGroup);
    void FreeGroupBuffers(int nGroup);
    void StopEntry(int nSlot);
    int  IsPlaying(int nVoice);

    // Silent while wNoSound is set.
    void PlayGroup(int nGroup) override;
    void PlaySpecific(int nVoice) override;
    int  LoadSound(struct SoundEntry* pSound);

    // Loads the sound list and opens the sound device.
    void Init() override;

    void PlayCDTrack(int nTrack) override;

    int                           nInitTimeMs; // 0x004, how long Init took
    std::unique_ptr<SoundManager> pEntries;    // 0x008
    // Set when DirectSound fails to start.
    short wNoSound; // 0x00c
    // Set when there are no music tracks to play.
    short wNoCD;   // 0x00e
    int   nUnused; // 0x014
};

// SYNTHETIC: HYPERX 0x00490860
// AudioMgr::`scalar deleting destructor'

#endif
