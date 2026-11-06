// The binding every engine object owns: 0x78c bytes, an abstract base, twenty blend slots, a tail.

#ifndef HYPERBLADE_ANIMBINDING_H
#define HYPERBLADE_ANIMBINDING_H

#include "anim/animplaystate.h"
#include "common/staticassert.h"
#include "math/vector.h"

// The play directions, as DATA/ENUMIDS.H's DirectionFlag numbers them.
#define A_FORWARD 1
#define A_REVERSE 2

class EngineObject;
class AnimNode;

#define ANIM_DB_BINDING_BYTES 0x78c
#define BINDING_SLOTS         20
#define BINDING_HEAD_BYTES    0x14

// This table fills two slots and leaves fourteen __purecall; 0x004b2974 fills all sixteen.
// VTABLE: HYPERX 0x004b29b4
class AnimBinding
{
public:
    // FUNCTION: HYPERX 0x00425450
    virtual ~AnimBinding()
    {
    }

    virtual AnimBinding*    DestroyViaVtable();
    virtual char            UpdateTick(void)                                                                                         = 0;
    virtual char            ClearAnimationSet()                                                                                      = 0;
    virtual char            StartAnimSetAtRate(class AnimDB* pAnimSet, float flRate)                                                 = 0;
    virtual char            StartClipPlayback(class AnimDB* pClip, float flRate, int nFlags)                                         = 0;
    virtual char            SelectAnimationSet(class AnimDB* pAnimSet, float flRate, int nDeferred, class AnimNamedRef* pTransition) = 0;
    virtual char            StartAnimSetDefaultRate(class AnimDB* pAnimSet)                                                          = 0;
    virtual char __fastcall AssignAnimEventSlot(int nChannel, int nTarget, int nMessage, int nParam)                                 = 0;
    virtual int __fastcall  SetPlayDirection(int nDirection)                                                                         = 0;
    virtual char __fastcall SetPlayState(int nState)                                                                                 = 0;
    virtual float           GetPlaybackRate()                                                                                        = 0;
    virtual void            SetPlaybackRate(float flRate)                                                                            = 0;
    virtual int             GetPlayState()                                                                                           = 0;
    virtual void __fastcall SetClipDuration(short nIndex, float flDuration)                                                          = 0;
    virtual void            UpdateFrame(char bAdvance)                                                                               = 0;

    AnimBinding();

    int                 m_nUnused;    // 0x004
    int                 m_nFrameInt;  // 0x008
    class EngineObject* m_pObject;    // 0x00c
    short               m_wBoneCount; // 0x010
};

// SYNTHETIC: HYPERX 0x00425460
// AnimBinding::`scalar deleting destructor'

STATIC_ASSERT(sizeof(AnimBinding) == BINDING_HEAD_BYTES);

inline AnimBinding::AnimBinding()
: m_nUnused(0)
, m_nFrameInt(0)
, m_pObject(0)
, m_wBoneCount(0)
{
}

// Slot one of this table and AnimPlayer's is PooledOwnerRoot::Destroy's folded body, which holds the marker.
inline AnimBinding* AnimBinding::DestroyViaVtable()
{
    if (this != 0)
    {
        delete this;
    }

    return 0;
}

#endif
