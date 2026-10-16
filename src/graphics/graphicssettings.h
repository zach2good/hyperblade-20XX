// Which renderer draws the 3D scene, and the hardware renderer's switches. They are saved under
// [Graphics] in booie.ini; the software renderer ignores every switch but the renderer itself.

#ifndef HYPERBLADE_GRAPHICSSETTINGS_H
#define HYPERBLADE_GRAPHICSSETTINGS_H

namespace GraphicsSettings
{

enum class Renderer
{
    SOFTWARE,
    HARDWARE
};

// Hardware only when a GPU device could be made; software otherwise, whatever was saved.
Renderer Active();

// Swaps the renderer and saves the choice; does nothing without a GPU device.
void ToggleRenderer();

// Whether the hardware renderer draws the textured models with their textures, or the shaded
// models in their palette colours as the software renderer does.
bool Textures();
void ToggleTextures();

// Nearest or linear texture filtering, as the original's F7 toggled.
bool LinearFiltering();
void ToggleLinearFiltering();

// Draws a match as often as the loop runs, blending between the 30 Hz ticks, or once a tick.
bool Uncapped();
void ToggleUncapped();

// The textured models at their most detailed, or stepped down with distance as the engine picks.
bool FullDetail();
void ToggleFullDetail();

// Faces lit flat, as the original did, or smoothly across the edges that do not crease.
bool SmoothShading();
void ToggleSmoothShading();

} // namespace GraphicsSettings

#endif
