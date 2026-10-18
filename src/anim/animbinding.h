// The binding every engine object owns: 0x78c bytes, an abstract base, twenty blend slots, a tail.

#ifndef HYPERBLADE_ANIMBINDING_H
#define HYPERBLADE_ANIMBINDING_H

#include "anim/animplaystate.h"
#include "math/vector.h"

// The play directions, as DATA/ENUMIDS.H's DirectionFlag numbers them.
constexpr auto A_FORWARD = 1;
constexpr auto A_REVERSE = 2;

class EngineObject;
class AnimNode;

constexpr auto ANIM_DB_BINDING_BYTES = 0x78c;
constexpr auto BINDING_SLOTS         = 20;
constexpr auto BINDING_HEAD_BYTES    = 0x208;

// This table fills two slots and leaves fourteen __purecall; 0x004b2974 fills all sixteen.
// VTABLE: HYPERX 0x004b29b4
class AnimBinding
{
public:
    // FUNCTION: HYPERX 0x00425450
    virtual ~AnimBinding() = default;

    virtual AnimBinding* DestroyViaVtable();
    virtual char         UpdateTick()                                                                                             = 0;
    virtual char         ClearAnimationSet()                                                                                      = 0;
    virtual char         StartAnimSetAtRate(class AnimDB* pAnimSet, float flRate)                                                 = 0;
    virtual char         StartClipPlayback(class AnimDB* pClip, float flRate, int nFlags)                                         = 0;
    virtual char         SelectAnimationSet(class AnimDB* pAnimSet, float flRate, int nDeferred, class AnimNamedRef* pTransition) = 0;
    virtual char         StartAnimSetDefaultRate(class AnimDB* pAnimSet)                                                          = 0;
    virtual char         AssignAnimEventSlot(int nChannel, int nTarget, int nMessage, int nParam)                                 = 0;
    virtual int          SetPlayDirection(int nDirection)                                                                         = 0;
    virtual char         SetPlayState(int nState)                                                                                 = 0;
    virtual float        GetPlaybackRate()                                                                                        = 0;
    virtual void         SetPlaybackRate(float flRate)                                                                            = 0;
    virtual int          GetPlayState()                                                                                           = 0;
    virtual void         SetClipDuration(short nIndex, float flDuration)                                                          = 0;
    virtual void         UpdateFrame(char bAdvance)                                                                               = 0;

    AnimBinding();

    int                 m_nUnused;    // 0x004
    int                 m_nFrameInt;  // 0x008
    class EngineObject* m_pObject;    // 0x00c
    short               m_wBoneCount; // 0x010
    class AnimDB*       m_pSet;       // 0x014
    // AnimPlayer::SelectAnimationSet moves m_pSet here before it takes the new one.
    class AnimDB* m_pPrevSet; // 0x018
    // The animation node the player is running, taken straight out of the set's node array.
    class AnimNode* m_pClip; // 0x01c
    // The previous clip; nothing reads a field through it.
    class AnimNode* m_pPrevClip; // 0x020
    // The clip set the player walks: the set the registry loads, one AnimClip per bone.
    class AnimClipSet* m_pBoneSet; // 0x024
    // Stored and never read.
    class AnimClipSet* m_pPrevBoneSet; // 0x028
    AnimPlayState      m_state;        // 0x02c
    AnimPlayState      m_saved;        // 0x108
    float              m_flDuration;   // 0x1e4
    float              m_flScale;      // 0x1e8
    float              m_flAccum;      // 0x1ec
    Vec3               m_vPos;         // 0x1f0
    Vec3               m_vDelta;       // 0x1fc
};

// SYNTHETIC: HYPERX 0x00425460
// AnimBinding::`scalar deleting destructor'

inline AnimBinding::AnimBinding()
: m_nUnused(0)
, m_nFrameInt(0)
, m_pObject(nullptr)
, m_wBoneCount(0)
{
    m_pSet         = nullptr;
    m_pPrevSet     = nullptr;
    m_pClip        = nullptr;
    m_pPrevClip    = nullptr;
    m_pBoneSet     = nullptr;
    m_pPrevBoneSet = nullptr;
}

// Slot one of this table and AnimPlayer's is PooledOwnerRoot::Destroy's folded body, which holds the marker.
inline AnimBinding* AnimBinding::DestroyViaVtable()
{
    delete this;

    return nullptr;
}

#endif
