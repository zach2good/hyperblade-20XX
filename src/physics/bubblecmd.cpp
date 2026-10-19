#include "common/compat.h"
#include "physics/boundbubble.h"
#include "physics/physicsbody.h"
#include "engine/collisionobjectids.h"

// The command record a bubble starts from: kind kCOBNone and everything else cleared.
// FUNCTION: HYPERX 0x0042a050
BubbleCmd::BubbleCmd()
{
    m_nSlot       = kCOBNone;
    m_wShape      = BubbleShape::kNone;
    m_nChildCount = 0;
    m_bUseDetail  = false;
}

// FUNCTION: HYPERX 0x0042a0a0
BubbleCmd::~BubbleCmd()
{
    // One link at a time, so a long chain can't overflow the stack.
    auto pChild = std::move(m_pChildren);

    while (pChild != nullptr)
    {
        pChild = std::move(pChild->m_pChildren);
    }
}
