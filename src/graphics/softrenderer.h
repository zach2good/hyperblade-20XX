#ifndef HYPERBLADE_SOFTRENDERER_H
#define HYPERBLADE_SOFTRENDERER_H

#include "common/compat.h"
#include "common/record.h"

struct RLESprite;

namespace SoftRenderer
{

int               ResizeBackBuffer(int* pWidth, int* pHeight);
void              FreeBackBuffer();
Record<RLESprite> LoadRLESprite(const char* pszName);

} // namespace SoftRenderer

#endif
