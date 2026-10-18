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
    m_pChildren   = nullptr;
    m_pAttach     = nullptr;
    m_pPos        = nullptr;
}

// FUNCTION: HYPERX 0x0042a0a0
BubbleCmd::~BubbleCmd()
{
    delete m_pPos;

    if (m_wShape == BubbleShape::kAttached && m_pAttach != nullptr)
    {
        delete m_pAttach;
    }

    delete m_pChildren;
}
