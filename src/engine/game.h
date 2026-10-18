#ifndef HYPERBLADE_GAME_H
#define HYPERBLADE_GAME_H

#include "common/compat.h"

namespace Game
{

void                    InitMatchMarkers();
extern "C" unsigned int SetEngineState(unsigned int state);
void                    RunFrame();

// RunFrame in two halves: one simulation tick, and one frame drawn at flAlpha between the last
// two ticks.
void         Tick();
void         Draw(float flAlpha);
void         InitializeEngine();
void         ShutdownEngine();
unsigned int UpdateState();

} // namespace Game

#endif
