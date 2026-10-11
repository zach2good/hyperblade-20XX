#ifndef HYPERBLADE_SOFTRENDERER_H
#define HYPERBLADE_SOFTRENDERER_H

#include "common/compat.h"

struct RLESprite;

namespace SoftRenderer
{

int                ResizeBackBuffer(int* pWidth, int* pHeight);
RLESprite* __cdecl LoadRLESprite(const char* pszName);

} // namespace SoftRenderer

#endif
