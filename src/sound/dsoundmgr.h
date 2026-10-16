#ifndef HYPERBLADE_DSOUNDMGR_H
#define HYPERBLADE_DSOUNDMGR_H
#include "common/compat.h"

#include "sound/soundchannel.h"

#define DSOUND_CHANNEL_COUNT 8

class DSoundMgr
{
public:
    static void __stdcall TimerProc(HWND, UINT, UINT_PTR, DWORD);

public:
    DSoundMgr();
    ~DSoundMgr();

    class SoundChannel* FindFreeChannel();

    // A sound with no free channel is dropped without complaint.
    void Play(struct SoundEntry* pSound);
    void StopEntry(struct SoundEntry* pOwner);
    void Shutdown();

    // One when there is a device and zero when not; every failure inside is caught here.
    int Init(HWND hWindow);

    // Neither reads the object: they work on g_mixerWaveOut and the options.
    int ReadSfxVolume();
    int FadeSfxVolume(int nStep);

    void Update();

    void StopAll();

private:
    IDirectSound*       m_pDevice;                         // 0x000
    IDirectSoundBuffer* m_pPrimary;                        // 0x004
    class SoundChannel  m_aChannels[DSOUND_CHANNEL_COUNT]; // 0x008
    unsigned int        m_nTimerId;                        // 0x2e8
    HWND                m_hWnd;                            // 0x2ec
    int                 m_nUnused;                         // 0x2f0
};

#endif
