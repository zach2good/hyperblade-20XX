#ifndef HYPERBLADE_STUB_H
#define HYPERBLADE_STUB_H

#include "common/compat.h"

namespace Stub
{

void Assert(int bCondition);

}

// The release build's assertion still calls an empty function.
#define RELEASE_ASSERT(bCondition) Stub::Assert(bCondition)

#endif
