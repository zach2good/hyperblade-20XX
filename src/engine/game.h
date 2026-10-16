#ifndef HYPERBLADE_GAME_H
#define HYPERBLADE_GAME_H

#include "common/compat.h"

namespace Game
{

void                    InitMatchMarkers(void);
extern "C" unsigned int SetEngineState(unsigned int state);
void __stdcall          RunFrame(void);

// RunFrame in two halves: one simulation tick, and one frame drawn at flAlpha between the last
// two ticks.
void                   Tick();
void                   Draw(float flAlpha);
void                   InitializeEngine(HWND hWnd, HINSTANCE hInstance, HPALETTE hPalette, HDC hSoftBackDC);
void __cdecl           ShutdownEngine(void);
unsigned int __stdcall UpdateState(void);

} // namespace Game

#endif
