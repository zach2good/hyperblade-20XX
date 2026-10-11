// The bubble node's constructor and destructor, compiled /Od like the original.

#include "common/compat.h"
#include "engine/devicector.h"
#include "physics/boundbubble.h"
#include "plannode.h"
#include "physics/collisioncage.h"
#include "physics/physicsbody.h"

#pragma optimize("", off)

// FUNCTION: HYPERX 0x0042a150
BubbleNode::BubbleNode()
{
    m_nShape      = BUBBLE_SHAPE_SPHERE;
    m_pOwner      = 0;
    m_nSlot       = -1;
    m_pTarget     = 0;
    m_pChildren   = 0;
    m_nChildCount = 0;
    m_pAttach     = 0;
    m_pCage       = 0;
}

// FUNCTION: HYPERX 0x0042a1f0
BubbleNode::~BubbleNode()
{
    if (m_nShape == BUBBLE_SHAPE_ATTACHED && m_pAttach != 0)
    {
        delete m_pAttach;
    }

    if (m_pCage != 0)
    {
        delete m_pCage;

        m_pCage = 0;
    }
}

#pragma optimize("", on)
