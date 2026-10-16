// The hardware renderer: draws the 3D scene on the GPU device the presenter's renderer runs on.

#ifndef HYPERBLADE_HWRENDERER_H
#define HYPERBLADE_HWRENDERER_H

#include "common/handle.h"

namespace HwRenderer
{

using Renderer = Handle<struct HwRendererTag>;
using Texture  = Handle<struct HwTextureTag>;

// Draws the scene at the asked size and returns it as a texture of the presenter's renderer, or an
// empty handle when it could not.
Texture Render(Renderer renderer, int nWidth, int nHeight);

} // namespace HwRenderer

#endif
