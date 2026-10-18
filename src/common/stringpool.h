#ifndef HYPERBLADE_STRINGPOOL_H
#define HYPERBLADE_STRINGPOOL_H

#include "common/compat.h"
#include "common/pathstring.h"
#include <array>
#include <vector>

struct StringPool
{
    static constexpr auto kBufferSize = 0x100;

    explicit StringPool(short nCount);

    short                                       wIndex;    // 0x00
    std::vector<std::array<char, kBufferSize> > apBuffers; // 0x04
    short                                       wCount;    // 0x08
    // The pool's own string, and the one the application formats its error messages into.
    String m_strMessage; // 0x0c

    // Not named LoadString: windows.h makes that a macro for LoadStringA.
    char* LoadText(unsigned int uID);
};

// FUNCTION: HYPERX 0x00480af0
// StringPool::~StringPool

#endif
