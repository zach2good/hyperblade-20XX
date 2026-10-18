#include "common/compat.h"
#include "common/animnamedref.h"
#include <cstring>
#include <memory>

// FUNCTION: HYPERX 0x00417880
AnimNamedRef::AnimNamedRef(const char* pszName, short nSegment, short nFrame)
{
    m_nFrame = nFrame;

    m_pszName = std::make_unique_for_overwrite<char[]>(strlen(pszName) + 1);
    strcpy(m_pszName.get(), pszName);

    const auto nAdjusted = nSegment + kAnimNamedRefBias;

    m_nSegment = static_cast<short>(nAdjusted);
}
