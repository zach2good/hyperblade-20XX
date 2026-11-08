// The plan parser's comment skipper, which only v1.0.0.0 has. It is not a member, so planvm.h
// keeps v1.0.0.1's line numbers and VC6's code for its inline functions does not move.

#ifdef HYPERX_V100
#include "planvm/planvm.h"

namespace PlanVMV100
{

// A group skips to the character after its ')'; a block comment's scan stops early on any '*'.
// FUNCTION: HYPERX100 0x00489b80
short SkipComment(PlanVM* pVM)
{
    const char* const pAt = &pVM->m_pszText[pVM->m_nCursor];

    if (pAt[0] == '/' && pAt[1] == '/')
    {
        pVM->SkipLine();

        return 0;
    }

    if (pAt[0] == '(')
    {
        while (pVM->m_pszText[pVM->m_nCursor - 1] != ')')
        {
            pVM->m_nCursor++;
        }
    }
    else if (pAt[1] == '*' && pAt[-2] != '*')
    {
        while (pVM->m_pszText[pVM->m_nCursor - 1] != '/')
        {
            pVM->m_nCursor++;

            if (pVM->m_pszText[pVM->m_nCursor - 2] == '*')
            {
                return 0;
            }
        }
    }

    return 0;
}

} // namespace PlanVMV100
#endif
