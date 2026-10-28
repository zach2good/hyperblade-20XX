// Tools that take over the screen from the menus or the match while they are open: a model viewer,
// a sound player and an image browser. The game underneath waits until the scene closes.

#ifndef HYPERBLADE_SCENES_H
#define HYPERBLADE_SCENES_H

namespace Scenes
{

enum class Scene
{
    None,
    ModelViewer,
    SoundPlayer,
    ImageBrowser,
};

// Scene::None closes the open one. The change happens at the next Frame.
void  Open(Scene scene);
Scene Current();

// Draws and presents one frame in place of the game's. False when no scene wants the frame.
bool Frame();

// The open scene's windows, inside the debug menu's ImGui frame.
void DrawWindows();

} // namespace Scenes

#endif
