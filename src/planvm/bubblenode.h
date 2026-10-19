#ifndef HYPERBLADE_BUBBLENODE_H
#define HYPERBLADE_BUBBLENODE_H

#include <memory>

struct Vec3;

class EngineObject;
class BoundBubble;
enum class BubbleShape : short;

// VTABLE: HYPERX 0x004b2a2c
class BubbleNode
{
public:
    BubbleNode();

    virtual ~BubbleNode();

    BubbleNode(const BubbleNode&)            = delete;
    BubbleNode(BubbleNode&&)                 = delete;
    BubbleNode& operator=(const BubbleNode&) = delete;
    BubbleNode& operator=(BubbleNode&&)      = delete;

    virtual void          GetWorldPosition(Vec3* pOut)       = 0;
    virtual unsigned char DoesIntersect(BoundBubble* pOther) = 0;

    BubbleShape                       m_nShape;      // 0x004
    class EngineObject*               m_pOwner;      // 0x008
    short                             m_nSlot;       // 0x00c
    class TransformObject*            m_pTarget;     // 0x010
    BoundBubble*                      m_pChildren;   // 0x014
    short                             m_nChildCount; // 0x018
    std::unique_ptr<struct AttachPak> m_pAttach;     // 0x01c
    // Taken from BubbleCmd::m_bUseDetail.
    bool                                  m_bUseDetail;
    std::unique_ptr<struct CollisionCage> m_pCage; // 0x020
};

#endif
