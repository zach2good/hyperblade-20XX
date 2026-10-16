// The hardware renderer: draws the 3D scene on the GPU device the presenter's renderer runs on.

#ifndef HYPERBLADE_HWRENDERER_H
#define HYPERBLADE_HWRENDERER_H

#include "common/handle.h"

struct MeshBlock;
struct MeshMaterial;

namespace HwRenderer
{

using Renderer = Handle<struct HwRendererTag>;
using Texture  = Handle<struct HwTextureTag>;

// One mesh block to draw this frame, in its camera's viewport within the back buffer.
struct Draw
{
    const MeshBlock*    pBlock;
    const MeshMaterial* pMaterials;
    int                 nMaterialCount;

    // The engine's pixel projection times the bone's world matrix, row major on column vectors.
    float aPixelFromModel[16];

    // The light the block is lit by, in the model's space.
    float aLightDir[4];

    int nViewportX;
    int nViewportY;
    int nViewportWidth;
    int nViewportHeight;

    // The original paints the arena first and every object over it, so objects are depth tested
    // only against each other and the arena never hides them.
    bool bArena;
};

// While set, the engine hands each mesh block to Submit instead of rasterising it.
bool Capturing();
void Submit(const Draw& draw);

// The model database loaded a model, so a freed mesh block's address may now hold another.
void ForgetMeshes();

// Draws the frame's submissions at the asked size and returns them as a texture of the
// presenter's renderer, or an empty handle when it could not. The palette is 256 entries of
// blue, green, red and a spare byte, as a DIB colour table holds them.
Texture Render(Renderer renderer, int nWidth, int nHeight, const unsigned char* pPalette);

} // namespace HwRenderer

#endif
