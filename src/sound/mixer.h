// The mixer and the volume fader it found, which are one object.

#ifndef HYPERBLADE_MIXER_H
#define HYPERBLADE_MIXER_H

#include "common/compat.h"
#include <mmsystem.h>

// The volume steps the options store.
#define MIXER_VOLUME_LEVELS 8

class Mixer
{
public:
    Mixer()
    : m_hMixer(0)
    {
    }

    ~Mixer()
    {
        if (m_hMixer != 0)
        {
            ::mixerClose(m_hMixer);
            m_hMixer = 0;
        }
    }

    // The wave-out component carries the sound effects and the compact-disc one the music.
    int Open(unsigned int nComponent);
    int FadeVolume(int nVolume);
    int ReadVolume();

    HMIXER       m_hMixer;  // 0x000
    MIXERLINEA   m_line;    // 0x004
    MIXERCONTROL m_control; // 0x0ac
};

#endif
