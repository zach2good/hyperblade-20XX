#include "common/compat.h"
#include <new.h>
#include "animdb.h"
#include "animnode.h"

// FUNCTION: HYPERX 0x00416f30
AnimNode::AnimNode(char bHasDescription)
{
    m_nUnused = 0;

    if (bHasDescription != 0)
    {
        m_pszDescription = new char[DESCRIPTION_CHARS + 1];
    }
    else
    {
        m_pszDescription = 0;
    }
}
