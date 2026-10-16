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

enum class Switch
{
    // The textured models with their textures, or the shaded models in their palette colours.
    TEXTURES,

    // Nearest or linear texture filtering, as the original's F7 toggled.
    LINEAR_FILTERING,

    // Draws a match as often as the loop runs, blending between the 30 Hz ticks, or once a tick.
    UNCAPPED,

    // The textured models at their most detailed, or stepped down with distance as the engine picks.
    FULL_DETAIL,

    // Faces lit flat, as the original did, or smoothly across the edges that do not crease:
    // Gouraud lights each corner and blends, Phong lights each pixel from the blended normal.
    SMOOTH_SHADING,
    PHONG_SHADING,

    // The scene's three lights, at azimuths 303, 54 and 180. The original lights each face by
    // one of them; a light switched off lights nothing.
    LIGHT_303,
    LIGHT_54,
    LIGHT_180,

    // Each face lit by the one light the original picks, or filled in by the other lights too.
    SEVERAL_LIGHTS,

    // A point light carried by the rok, added over the rest.
    ROK_LIGHT,

    // The two lights above the arena cast shadow maps, in place of the original's shadow meshes.
    SHADOW_MAPS,

    // Each mesh drawn over the ones sorted behind it, as the original painted them, or every mesh
    // depth tested against the others.
    PAINTERS_ORDER,

    COUNT
};

bool On(Switch which);
void Toggle(Switch which);

// How far the rok's light reaches, in world units. Not saved; the debug menu tunes it.
float RokLightRadius();
void  SetRokLightRadius(float flRadius);

} // namespace GraphicsSettings

#endif
