#include "common/compat.h"
#include "input/keymap.h"
#include <SDL3/SDL.h>
#include <utility>

namespace
{

constexpr std::pair<SDL_Scancode, unsigned int> KEYS[] = {
    { SDL_SCANCODE_RETURN, VK_RETURN },
    { SDL_SCANCODE_ESCAPE, VK_ESCAPE },
    { SDL_SCANCODE_BACKSPACE, VK_BACK },
    { SDL_SCANCODE_TAB, VK_TAB },
    { SDL_SCANCODE_SPACE, VK_SPACE },
    { SDL_SCANCODE_MINUS, VK_OEM_MINUS },
    { SDL_SCANCODE_EQUALS, VK_OEM_PLUS },
    { SDL_SCANCODE_LEFTBRACKET, VK_OEM_4 },
    { SDL_SCANCODE_RIGHTBRACKET, VK_OEM_6 },
    { SDL_SCANCODE_BACKSLASH, VK_OEM_5 },
    { SDL_SCANCODE_SEMICOLON, VK_OEM_1 },
    { SDL_SCANCODE_APOSTROPHE, VK_OEM_7 },
    { SDL_SCANCODE_GRAVE, VK_OEM_3 },
    { SDL_SCANCODE_COMMA, VK_OEM_COMMA },
    { SDL_SCANCODE_PERIOD, VK_OEM_PERIOD },
    { SDL_SCANCODE_SLASH, VK_OEM_2 },
    { SDL_SCANCODE_NONUSBACKSLASH, VK_OEM_102 },
    { SDL_SCANCODE_CAPSLOCK, VK_CAPITAL },
    { SDL_SCANCODE_F1, VK_F1 },
    { SDL_SCANCODE_F2, VK_F2 },
    { SDL_SCANCODE_F3, VK_F3 },
    { SDL_SCANCODE_F4, VK_F4 },
    { SDL_SCANCODE_F5, VK_F5 },
    { SDL_SCANCODE_F6, VK_F6 },
    { SDL_SCANCODE_F7, VK_F7 },
    { SDL_SCANCODE_F8, VK_F8 },
    { SDL_SCANCODE_F9, VK_F9 },
    { SDL_SCANCODE_F11, VK_F11 },
    { SDL_SCANCODE_F12, VK_F12 },
    { SDL_SCANCODE_PRINTSCREEN, VK_SNAPSHOT },
    { SDL_SCANCODE_SCROLLLOCK, VK_SCROLL },
    { SDL_SCANCODE_PAUSE, VK_PAUSE },
    { SDL_SCANCODE_INSERT, VK_INSERT },
    { SDL_SCANCODE_HOME, VK_HOME },
    { SDL_SCANCODE_PAGEUP, VK_PRIOR },
    { SDL_SCANCODE_DELETE, VK_DELETE },
    { SDL_SCANCODE_END, VK_END },
    { SDL_SCANCODE_PAGEDOWN, VK_NEXT },
    { SDL_SCANCODE_RIGHT, VK_RIGHT },
    { SDL_SCANCODE_LEFT, VK_LEFT },
    { SDL_SCANCODE_DOWN, VK_DOWN },
    { SDL_SCANCODE_UP, VK_UP },
    { SDL_SCANCODE_NUMLOCKCLEAR, VK_NUMLOCK },
    { SDL_SCANCODE_KP_DIVIDE, VK_DIVIDE },
    { SDL_SCANCODE_KP_MULTIPLY, VK_MULTIPLY },
    { SDL_SCANCODE_KP_MINUS, VK_SUBTRACT },
    { SDL_SCANCODE_KP_PLUS, VK_ADD },
    { SDL_SCANCODE_KP_ENTER, VK_RETURN },
    { SDL_SCANCODE_KP_1, VK_NUMPAD1 },
    { SDL_SCANCODE_KP_2, VK_NUMPAD2 },
    { SDL_SCANCODE_KP_3, VK_NUMPAD3 },
    { SDL_SCANCODE_KP_4, VK_NUMPAD4 },
    { SDL_SCANCODE_KP_5, VK_NUMPAD5 },
    { SDL_SCANCODE_KP_6, VK_NUMPAD6 },
    { SDL_SCANCODE_KP_7, VK_NUMPAD7 },
    { SDL_SCANCODE_KP_8, VK_NUMPAD8 },
    { SDL_SCANCODE_KP_9, VK_NUMPAD9 },
    { SDL_SCANCODE_KP_0, VK_NUMPAD0 },
    { SDL_SCANCODE_KP_PERIOD, VK_DECIMAL },
    { SDL_SCANCODE_APPLICATION, VK_APPS },
    { SDL_SCANCODE_LCTRL, VK_CONTROL },
    { SDL_SCANCODE_RCTRL, VK_CONTROL },
    { SDL_SCANCODE_LSHIFT, VK_SHIFT },
    { SDL_SCANCODE_RSHIFT, VK_SHIFT },
    { SDL_SCANCODE_LGUI, VK_LWIN },
    { SDL_SCANCODE_RGUI, VK_RWIN },
};

} // namespace

unsigned int KeyMap::ToVirtualKey(int nScancode, int nKeycode)
{
    if (nKeycode >= SDLK_A && nKeycode <= SDLK_Z)
    {
        return 'A' + (unsigned int)(nKeycode - SDLK_A);
    }

    // SDL numbers the top row 1 to 9, then 0.
    if (nScancode >= SDL_SCANCODE_1 && nScancode <= SDL_SCANCODE_9)
    {
        return '1' + (unsigned int)(nScancode - SDL_SCANCODE_1);
    }

    if (nScancode == SDL_SCANCODE_0)
    {
        return '0';
    }

    for (const auto& [scancode, nKey] : KEYS)
    {
        if (scancode == nScancode)
        {
            return nKey;
        }
    }

    return 0;
}
