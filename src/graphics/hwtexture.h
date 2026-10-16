// The hardware renderer's textures: each model texture uploaded once, with its mipmaps.

#ifndef HYPERBLADE_HWTEXTURE_H
#define HYPERBLADE_HWTEXTURE_H

#include "graphics/hwpipeline.h"

class TexCacheEntry;

namespace HwTexture
{

using Texture = Handle<struct HwGPUTextureTag>;

// The entry's bitmap from the textures folder, uploaded the first time its name is asked for.
// A file that will not load is magenta, and a null entry is white.
Texture Find(HwPipeline::Device device, const TexCacheEntry* pEntry);

} // namespace HwTexture

#endif
