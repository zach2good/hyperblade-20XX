// The bubble node's constructor and destructor, compiled /Od like the original.

#include "common/compat.h"
#include "engine/devicector.h"
#include "physics/boundbubble.h"
#include "plannode.h"
#include "physics/collisioncage.h"
#include "physics/physicsbody.h"

// FUNCTION: HYPERX 0x0042a150
BubbleNode::BubbleNode()
{
    m_nShape      = BubbleShape::kSphere;
    m_pOwner      = nullptr;
    m_nSlot       = -1;
    m_pTarget     = nullptr;
    m_pChildren   = nullptr;
    m_nChildCount = 0;
    m_pAttach     = nullptr;
    m_pCage       = nullptr;
}

// FUNCTION: HYPERX 0x0042a1f0
BubbleNode::~BubbleNode()
{
    if (m_nShape == BubbleShape::kAttached && m_pAttach != nullptr)
    {
        delete m_pAttach;
    }
}
