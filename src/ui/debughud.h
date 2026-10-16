#ifndef HYPERBLADE_DEBUGHUD_H
#define HYPERBLADE_DEBUGHUD_H

#include "common/compat.h"

namespace DebugHUD
{

void PushLightString(const char* pszSource, short nLength);
void UpdateLightingInfo(void);

} // namespace DebugHUD

#endif
