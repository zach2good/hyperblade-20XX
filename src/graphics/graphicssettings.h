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

} // namespace GraphicsSettings

#endif
