#ifndef HYPERBLADE_BUBBLENODE_H
#define HYPERBLADE_BUBBLENODE_H

struct Vec3;

class EngineObject;
class BoundBubble;

// VTABLE: HYPERX 0x004b2a2c
class BubbleNode
{
public:
    BubbleNode();

    virtual ~BubbleNode();
    virtual void          GetWorldPosition(Vec3* pOut)       = 0;
    virtual unsigned char DoesIntersect(BoundBubble* pOther) = 0;

    short                  m_nShape;      // 0x004
    class EngineObject*    m_pOwner;      // 0x008
    short                  m_nSlot;       // 0x00c
    class TransformObject* m_pTarget;     // 0x010
    BoundBubble*           m_pChildren;   // 0x014
    short                  m_nChildCount; // 0x018
    // An AttachPak for an attached bubble, else the parser's kExtraUseDetail sentinel.
    struct AttachPak*     m_pAttach; // 0x01c
    struct CollisionCage* m_pCage;   // 0x020
};

#endif
