#ifndef HYPERBLADE_PLAYER_H
#define HYPERBLADE_PLAYER_H

#include "common/compat.h"

class EngineObject;

namespace Player
{

void __cdecl          ProcessTrickInput(EngineObject* pPlayer, short* pnInput);
EngineObject* __cdecl FindBungeeVictim(EngineObject* pActor, float* pflOut);

} // namespace Player

#endif
