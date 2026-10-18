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
    void SetCallbackTarget(int nSlot, int nArgA, int nArgB);

    char SetClip(short nClipIndex, float flBlend, short nLoopStart);

    ~AnimPlayer() override;

    AnimBinding* DestroyViaVtable() override
    {
        delete this;

        return nullptr;
    }

    char UpdateTick(void) override;
    char ClearAnimationSet() override;
    char StartAnimSetAtRate(class AnimDB* pAnimSet, float flRate) override;

    // Slot five is EngineObject::PostEvent's folded body.
    char StartClipPlayback(AnimDB* pClip, float flRate, int nFlags) override
    {
        return SelectAnimationSet(pClip, flRate, nFlags, nullptr);
    }

    char  SelectAnimationSet(class AnimDB* pAnimSet, float flRate, int nDeferred, class AnimNamedRef* pTransition) override;
    char  StartAnimSetDefaultRate(class AnimDB* pAnimSet) override;
    char  AssignAnimEventSlot(int nChannel, int nTarget, int nMessage, int nParam) override;
    int   SetPlayDirection(int nDirection) override;
    char  SetPlayState(int nState) override;
    float GetPlaybackRate() override;
    void  SetPlaybackRate(float flRate) override;
    int   GetPlayState() override;
    void  SetClipDuration(short nIndex, float flDuration) override;
    void  UpdateFrame(char bAdvance) override;

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

// SYNTHETIC: HYPERX 0x00425560
// AnimPlayer::`scalar deleting destructor'

// The atexit thunk for ComputeScale's blend transform.
// SYNTHETIC: HYPERX 0x00424e50
// $E2

#endif
