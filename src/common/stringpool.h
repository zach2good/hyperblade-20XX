#ifndef HYPERBLADE_STRINGPOOL_H
#define HYPERBLADE_STRINGPOOL_H

#include "common/compat.h"
#include "common/pathstring.h"

struct StringPool
{
    explicit StringPool(short nCount);
    ~StringPool();

    short  wIndex;    // 0x00
    char** apBuffers; // 0x04
    short  wCount;    // 0x08
    // The pool's own string, and the one the application formats its error messages into.
    String m_strMessage; // 0x0c

    // Not named LoadString: windows.h makes that a macro for LoadStringA.
    char* LoadText(unsigned int uID);
};

#endif
