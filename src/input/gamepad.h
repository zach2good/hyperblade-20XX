// Gamepads through SDL. In a match a pad presses the player's keys; in the menus it sends the
// keys that move between and press the buttons.

#ifndef HYPERBLADE_GAMEPAD_H
#define HYPERBLADE_GAMEPAD_H

namespace Gamepad
{

void Init();
void Shutdown();

// A pad control and the action booie.ini [Gamepad] gives it.
struct Binding
{
    const char* pszControl;
    const char* pszAction;
};

int     BindingCount();
Binding GetBinding(int nIndex);

// The pad that sent input last, as an SDL joystick id; zero before any has.
unsigned int LastUsed();

} // namespace Gamepad

#endif
