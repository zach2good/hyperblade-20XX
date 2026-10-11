#ifndef HYPERBLADE_AUDIOMGRBASE_H
#define HYPERBLADE_AUDIOMGRBASE_H
#include "common/compat.h"

// Abstract: its table at 0x004b4098 is a destructor and five _purecall.
// VTABLE: HYPERX 0x004b4098
class AudioMgrBase
{
public:
    // FUNCTION: HYPERX 0x00491850
    virtual ~AudioMgrBase()
    {
    }

    virtual void Init(HWND hWindow)       = 0;
    virtual void PlayGroup(int nGroup)    = 0;
    virtual void PlaySpecific(int nVoice) = 0;
    virtual void PlayCDTrack(int nTrack)  = 0;
    virtual void StopCD()                 = 0;
};

// SYNTHETIC: HYPERX 0x00491860
// AudioMgrBase::`scalar deleting destructor'

#endif
