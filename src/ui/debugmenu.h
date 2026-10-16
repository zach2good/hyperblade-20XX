// The debug menu, an ImGui window over the game that holds the runtime switches. F2 opens and
// closes it.

#ifndef HYPERBLADE_DEBUGMENU_H
#define HYPERBLADE_DEBUGMENU_H

#include "common/handle.h"

namespace DebugMenu
{

using Event    = Handle<struct EventTag>;
using Renderer = Handle<struct RendererTag>;

void Toggle();

// Every window event goes to ImGui, so it knows the mouse and keys when the menu opens.
void OnEvent(Event event);

// While the menu is open and under the pointer or typing, the game does not see that input.
bool WantsMouse();
bool WantsKeyboard();

// Draws the menu over the finished frame, before it is presented.
void Draw(Renderer renderer);

} // namespace DebugMenu

#endif
