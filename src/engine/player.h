#ifndef HYPERBLADE_PLAYER_H
#define HYPERBLADE_PLAYER_H

#include "common/compat.h"

class EngineObject;

namespace Player
{

void          ProcessTrickInput(EngineObject* pPlayer, short* pnInput);
EngineObject* FindBungeeVictim(EngineObject* pActor, float* pflOut);

} // namespace Player

#endif
