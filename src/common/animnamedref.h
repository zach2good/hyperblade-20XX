#ifndef HYPERBLADE_ANIMNAMEDREF_H
#define HYPERBLADE_ANIMNAMEDREF_H

#include "staticassert.h"

// The segment carries this bias while the reference still owns its copied name.
#define ANIMNAMEDREF_BIAS 0x270f

class AnimNamedRef
{
public:
    AnimNamedRef(const char* pszName, short nSegment, short nFrame);

    ~AnimNamedRef();

    // The copied name until resolved, then the set it resolved to.
public:
    union
    {
        char*         m_pszName; // 0x00
        class AnimDB* m_pDb;
    };

    short m_nSegment; // 0x04
    short m_nFrame;   // 0x06
};

STATIC_ASSERT(sizeof(AnimNamedRef) == 0x8);
#endif
