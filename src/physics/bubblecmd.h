#ifndef HYPERBLADE_BUBBLECMD_H
#define HYPERBLADE_BUBBLECMD_H

#include "common/compat.h"
#include "physics/boundbubble.h"
#include <memory>

struct AttachPak;

// What a bubble is: the shape word every object definition's table gives its collision type.
enum class BubbleShape : short
{
    kNone     = 0,
    kSphere   = 1,
    kAttached = 4,
    kCaged    = 5
};

// One per bubble on an object definition; the parser makes each new bubble the last one's child.
class BubbleCmd
{
public:
    BubbleCmd();
    ~BubbleCmd();

    BubbleCmd(const BubbleCmd&)            = delete;
    BubbleCmd(BubbleCmd&&)                 = delete;
    BubbleCmd& operator=(const BubbleCmd&) = delete;
    BubbleCmd& operator=(BubbleCmd&&)      = delete;

    short       m_nSlot;       // 0x00
    BubbleShape m_wShape;      // 0x02
    short       m_nChildCount; // 0x04
    // The next command in the chain, the only child the parser gives.
    std::unique_ptr<BubbleCmd> m_pChildren; // 0x08
    // An AttachPak the bubble takes over.
    std::unique_ptr<AttachPak> m_pAttach; // 0x0c
    // A caged bubble with no attach point, which gets a collision cage on slot zero.
    bool                             m_bUseDetail;
    std::unique_ptr<BubbleShapeNode> m_pPos; // 0x10
};

#endif
