#ifndef HYPERBLADE_LEVEL_H
#define HYPERBLADE_LEVEL_H

#include "common/compat.h"

namespace Level
{

int __stdcall IndexStationaryObjects(void);
int __cdecl   ValidateArenaObjects(struct SyncBlocks* pLocal, struct SyncBlocks* pRemote);

} // namespace Level

#endif
