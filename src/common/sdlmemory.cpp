#include "common/sdlmemory.h"
#include <SDL3/SDL.h>

void SDLFree::operator()(void* pMemory) const noexcept
{
    SDL_free(pMemory);
}
