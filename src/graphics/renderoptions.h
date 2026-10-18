#ifndef HYPERBLADE_RENDEROPTIONS_H
#define HYPERBLADE_RENDEROPTIONS_H

// What the device can do, which gates the render menu's toggles.
struct RenderOptions
{
    int bTextured;
    int bPerspective;
    int bFiltered;
};

// Renderer::SetRenderOption's index; setting perspective also sets filtering.
enum class RenderOption
{
    kTextured,
    kPerspective,
    kFiltered
};

#endif
