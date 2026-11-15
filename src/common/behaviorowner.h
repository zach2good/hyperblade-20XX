#ifndef HYPERBLADE_BEHAVIOROWNER_H
#define HYPERBLADE_BEHAVIOROWNER_H

class GameBehaviorObject;

// A root class: with a base destructor to call, VC6 stops inlining this one into the deleter.
// VTABLE: HYPERX 0x004b2834
class BehaviorOwner
{
public:
    __forceinline BehaviorOwner()
    {
    }

    // FUNCTION: HYPERX100 0x004c6c10
    // FUNCTION: HYPERX 0x004209f0
    virtual ~BehaviorOwner()
    {
    }

    // Pure: every pool defines its own, and the linker folds the copies.
    virtual BehaviorOwner* Destroy() = 0;

    virtual void Init() = 0;

    virtual char Insert(void* pNode) = 0;

    // Answers with a node off the free list.
    virtual void* Alloc() = 0;

    GameBehaviorObject* m_pHead;
};

// SYNTHETIC: HYPERX 0x00420a00
// BehaviorOwner::`scalar deleting destructor'

#endif
