#include "common/compat.h"
#include "input/input.h"
#include <SDL3/SDL.h>

// FUNCTION: HYPERX 0x0044a120
int Input::IsShiftKeyDown()
{
    return (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
}

// FUNCTION: HYPERX 0x0044a140
int Input::IsCtrlKeyDown()
{
    return (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
}
