#ifndef HYPERBLADE_BUBBLECMD_H
#define HYPERBLADE_BUBBLECMD_H

#include "common/compat.h"
#include "physics/boundbubble.h"

struct AttachPak;

// What a bubble is: the shape word every object definition's table gives its collision type.
enum BubbleShape
{
    BUBBLE_SHAPE_SPHERE   = 1,
    BUBBLE_SHAPE_ATTACHED = 4,
    BUBBLE_SHAPE_CAGED    = 5
};

// The attach word of a caged bubble with no attach point; the low half is the cage's slot.
#define EXTRA_USE_DETAIL 0xabcd0000

// One per bubble on an object definition; the parser makes each new bubble the last one's child.
class BubbleCmd
{
public:
    BubbleCmd();
    ~BubbleCmd();

    short      m_nSlot;       // 0x00
    short      m_wShape;      // 0x02
    short      m_nChildCount; // 0x04
    BubbleCmd* m_pChildren;   // 0x08
    // An AttachPak the bubble takes over, or EXTRA_USE_DETAIL (0xabcd0000) when there is none.
    AttachPak*       m_pAttach; // 0x0c
    BubbleShapeNode* m_pPos;    // 0x10
};

#endif
