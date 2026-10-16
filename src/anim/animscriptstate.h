// The state the animation script parser carries between lines.

#ifndef HYPERBLADE_ANIMSCRIPTSTATE_H
#define HYPERBLADE_ANIMSCRIPTSTATE_H
#include "common/compat.h"
#include "common/string.h"

class AnimDB;
class AnimNode;

// The set filter and the clip name prefix are string resources 32 and 66.
struct AnimScriptState
{
    AnimScriptState(const char* pszFilter, const char* pszPrefix)
    : bStarted(0)
    , pDb(0)
    , pSegment(0)
    , strFilter(pszFilter)
    , strPrefix(pszPrefix)
    {
    }

    int       bStarted;  // 0x00
    AnimDB*   pDb;       // 0x04
    AnimNode* pSegment;  // 0x08
    String    strFilter; // 0x0c
    String    strPrefix; // 0x10
};

// The parser's unwind destroys a half-built state through the out-of-line copy.
// SYNTHETIC: HYPERX 0x00418ba0
// AnimScriptState::~AnimScriptState

#endif
