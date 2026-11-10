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
    const char* pszDefault;
};

int     BindingCount();
Binding GetBinding(int nIndex);

// Binds a control to a named action for this run and in booie.ini, as a hand edit would.
void SetBinding(int nIndex, const char* pszAction);

// The actions a control can take, by the names booie.ini [Gamepad] uses.
int         ActionCount();
const char* ActionName(int nIndex);

// The pad that sent input last, as an SDL joystick id; zero before any has.
unsigned int LastUsed();

} // namespace Gamepad

#endif
