#ifndef HYPERBLADE_AUDIOMGR_H
#define HYPERBLADE_AUDIOMGR_H

#include "sound/audiomgrbase.h"

// PlayCDTrack's tracks.
#define CD_TRACK_RANDOM (-1)
#define CD_TRACK_MENU   10

// VTABLE: HYPERX 0x004b4080
struct AudioMgr : public AudioMgrBase
{
    void StopAllSfx();

    AudioMgr();

    virtual ~AudioMgr();

    virtual void StopCD();

    int  SfxReadVolume();
    void SfxFadeVolume(int nVolume);
    int  CDReadVolume();
    void CDFadeVolume(int nVolume);

    int  PreloadGroup(int nGroup);
    void FreeGroupBuffers(int nGroup);
    void StopEntry(int nSlot);
    int  IsPlaying(int nVoice);

    // Silent while wNoSound is set.
    virtual void PlayGroup(int nGroup);
    virtual void PlaySpecific(int nVoice);
    int          LoadSound(struct SoundEntry* pSound);

    // Loads the sound list and opens the sound device.
    virtual void Init(HWND hWindow);

    virtual void PlayCDTrack(int nTrack);

    int nInitTimeMs; // 0x004, how long Init took
    // The sound manager, built with `new SoundManager`.
    class SoundManager* pEntries; // 0x008
    // Set when DirectSound fails to start.
    short wNoSound; // 0x00c
    // Set when there are no music tracks to play.
    short wNoCD;   // 0x00e
    HWND  hWnd;    // 0x010
    int   nUnused; // 0x014
};

// SYNTHETIC: HYPERX 0x00490860
// AudioMgr::`scalar deleting destructor'

#endif
