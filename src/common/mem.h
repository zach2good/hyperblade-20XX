#ifndef HYPERBLADE_MEM_H
#define HYPERBLADE_MEM_H

#include "common/compat.h"

namespace Mem
{

void __cdecl ZeroBlock192(void* pBlock);
void __cdecl ZeroBlockUnrolled128(void* pBlock);

} // namespace Mem

#endif
