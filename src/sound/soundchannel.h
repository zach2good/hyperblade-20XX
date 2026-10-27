// The streaming sound channel and the DirectSound buffer it fills.

#ifndef HYPERBLADE_SOUNDCHANNEL_H
#define HYPERBLADE_SOUNDCHANNEL_H
#include "common/compat.h"

#include "soundmanager.h"
#include <windows.h>
#include <dsound.h>

// DSBUFFERDESC as DirectX 7 declares it; this compiler's dsound.h ends before guid3DAlgorithm.
struct SoundBufferDesc
{
    DWORD          dwSize;          // 0x00
    DWORD          dwFlags;         // 0x04
    DWORD          dwBufferBytes;   // 0x08
    DWORD          dwReserved;      // 0x0c
    PCMWAVEFORMAT* pFormat;         // 0x10
    GUID           guid3DAlgorithm; // 0x14
};

// A drained channel has written its last sample and stops on the next update.
enum ChannelState
{
    CHANNEL_IDLE,
    CHANNEL_PLAYING,
    CHANNEL_DRAINED
};

class SoundChannel
{
public:
    int FillBuffer(unsigned int nBytes, unsigned int nOffset);

    void StreamUpdate();

    unsigned int StartPlayback(IDirectSound* pDevice, SoundEntry* pSource, int bLooping);

    unsigned int ReadFromFile(void* pBuffer, unsigned int nBytes);

    void Stop();

    SoundChannel();
    ~SoundChannel();

    // Public because DSoundMgr::FindFreeChannel and StopEntry read them off each entry.
    SoundEntry*         m_pSource; // 0x00
    ChannelState        m_nState;  // 0x04
    IDirectSoundBuffer* m_pBuffer; // 0x08
    // The format pointer in m_desc points at m_format, inside the channel itself.
    SoundBufferDesc m_desc;     // 0x0c
    PCMWAVEFORMAT   m_format;   // 0x30
    HANDLE          m_hFile;    // 0x40
    int             m_bLooping; // 0x44
    int             m_nWritten; // 0x48
    int             m_nTotal;   // 0x4c
    // The ring size for a stream; zero for a sound small enough to hold whole.
    int m_nRingSize; // 0x50
    int m_nRingHalf; // 0x54
    // Shared with a re-entered timer tick, so every read and write goes to memory.
    volatile int m_nBusy; // 0x58

    static int IsResidentGroup(const struct SoundEntry* pEntry, int nGroup);
};

#endif
