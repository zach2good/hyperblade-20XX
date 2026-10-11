#ifndef HYPERBLADE_ANIMPLAYER_H
#define HYPERBLADE_ANIMPLAYER_H

#include "common/compat.h"
#include "anim/animbinding.h"
#include "math/slerpseg.h"

class EngineObject;

// A set and rate SelectAnimationSet defers until the playing clip ends.
struct AnimPendingRequest
{
    class AnimDB* pSet;
    float         flRate;
    int           anUnused[10];
};

// Slots one and five hold bodies the linker folded onto other classes.
// VTABLE: HYPERX 0x004b2974
class AnimPlayer : public AnimBinding
{
public:
    void __fastcall SetCallbackTarget(int nSlot, int nArgA, int nArgB);

    char __fastcall SetClip(short nClipIndex, float flBlend, short nLoopStart);

    virtual ~AnimPlayer();

    virtual AnimBinding* DestroyViaVtable()
    {
        delete this;

        return 0;
    }

    virtual char UpdateTick(void);
    virtual char ClearAnimationSet();
    virtual char StartAnimSetAtRate(class AnimDB* pAnimSet, float flRate);

    // Slot five is EngineObject::PostEvent's folded body.
    virtual char StartClipPlayback(AnimDB* pClip, float flRate, int nFlags)
    {
        return SelectAnimationSet(pClip, flRate, nFlags, 0);
    }

    virtual char            SelectAnimationSet(class AnimDB* pAnimSet, float flRate, int nDeferred, class AnimNamedRef* pTransition);
    virtual char            StartAnimSetDefaultRate(class AnimDB* pAnimSet);
    virtual char __fastcall AssignAnimEventSlot(int nChannel, int nTarget, int nMessage, int nParam);
    virtual int __fastcall  SetPlayDirection(int nDirection);
    virtual char __fastcall SetPlayState(int nState);
    virtual float           GetPlaybackRate();
    virtual void            SetPlaybackRate(float flRate);
    virtual int             GetPlayState();
    virtual void __fastcall SetClipDuration(short nIndex, float flDuration);
    virtual void            UpdateFrame(char bAdvance);

    AnimPlayer();

    void SetObject(class EngineObject* pObject);
    char Advance(float flDelta);
    char ComputeScale(float flScale);
    char ConditionalDispatch(class AnimNamedRef* pTransition);

    static short TransitionClip(const class AnimNamedRef* pTransition);
    static short TransitionLoopStart(const class AnimNamedRef* pTransition);

    // The original's element destructor is Object::Trace, onto which its linker folded it.
    SlerpSeg m_aSlots[BINDING_SLOTS];

    // SelectAnimationSet parks the set and the rate here and raises m_bPending.
    AnimPendingRequest m_pending;  // 0x758
    short              m_bPending; // 0x788

    unsigned char CheckLoopPoint();

    char OnClipEnd();
};

STATIC_ASSERT(sizeof(AnimPlayer) == ANIM_DB_BINDING_BYTES);

// SYNTHETIC: HYPERX 0x00425560
// AnimPlayer::`scalar deleting destructor'

// The atexit thunk for ComputeScale's blend transform.
// SYNTHETIC: HYPERX 0x00424e50
// $E2

#endif
