#ifndef HYPERBLADE_GAME_H
#define HYPERBLADE_GAME_H

#include "common/compat.h"

namespace Game
{

void                    InitMatchMarkers(void);
extern "C" unsigned int SetEngineState(unsigned int state);
void __stdcall          RunFrame(void);
void                    InitializeEngine(HWND hWnd, HINSTANCE hInstance, HPALETTE hPalette, HDC hSoftBackDC);
void __cdecl            ShutdownEngine(void);
unsigned int __stdcall  UpdateState(void);

#ifdef HYPERX_V100
// The target goal slot of the player in the slot the viewed one is not, of slots 0 and 1.
short GetOpponentTargetGoalSlot(void);
short GetRokPrevCarrierSlot(void);
#endif

} // namespace Game

#endif
