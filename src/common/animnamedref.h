#ifndef HYPERBLADE_ANIMNAMEDREF_H
#define HYPERBLADE_ANIMNAMEDREF_H

#include <memory>

// The segment carries this bias while the reference still owns its copied name.
constexpr auto kAnimNamedRefBias = 0x270f;

class AnimNamedRef
{
public:
    AnimNamedRef(const char* pszName, short nSegment, short nFrame);

    // The copied name until resolved, then the set it resolved to; the original shares one slot.
public:
    std::unique_ptr<char[]> m_pszName; // 0x00
    class AnimDB*           m_pDb = nullptr;

    short m_nSegment; // 0x04
    short m_nFrame;   // 0x06
};

// FUNCTION: HYPERX 0x004178e0
// AnimNamedRef::~AnimNamedRef

#endif
