// Memory that SDL allocates and the caller must give back with SDL_free.

#ifndef HYPERBLADE_SDLMEMORY_H
#define HYPERBLADE_SDLMEMORY_H

#include <memory>

struct SDLFree
{
    void operator()(void* pMemory) const noexcept;
};

template <class T>
using SDLPtr = std::unique_ptr<T, SDLFree>;

#endif
