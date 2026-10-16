#ifndef HYPERBLADE_LEVEL_H
#define HYPERBLADE_LEVEL_H

#include "common/compat.h"

namespace Level
{

int IndexStationaryObjects(void);
int ValidateArenaObjects(struct SyncBlocks* pLocal, struct SyncBlocks* pRemote);

} // namespace Level

#endif
