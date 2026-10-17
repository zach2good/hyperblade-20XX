#include "common/compat.h"
#include "physics/boundbubble.h"
#include "physics/physicsbody.h"
#include "engine/collisionobjectids.h"

// The command record a bubble starts from: kind COB_NONE and everything else cleared.
// FUNCTION: HYPERX 0x0042a050
BubbleCmd::BubbleCmd()
{
    m_nSlot       = COB_NONE;
    m_wShape      = 0;
    m_nChildCount = 0;
    m_pChildren   = 0;
    m_pAttach     = 0;
    m_pPos        = 0;
}

// FUNCTION: HYPERX 0x0042a0a0
BubbleCmd::~BubbleCmd()
{
    if (m_pPos != 0)
    {
        delete m_pPos;
    }

    if (m_wShape == BUBBLE_SHAPE_ATTACHED && m_pAttach != 0)
    {
        delete m_pAttach;
    }

    if (m_pChildren != 0)
    {
        delete m_pChildren;
    }
}
