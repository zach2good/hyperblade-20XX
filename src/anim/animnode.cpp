#include "common/compat.h"
#include <memory>
#include "animdb.h"
#include "animnode.h"

// FUNCTION: HYPERX 0x00416f30
AnimNode::AnimNode(char bHasDescription)
{
    m_nUnused = 0;

    if (bHasDescription != 0)
    {
        m_pszDescription = std::make_unique_for_overwrite<char[]>(kDescriptionChars + 1);
    }
}
