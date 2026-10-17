#include "common/compat.h"
#include "input/keycodes.h"
#include "input/keymap.h"
#include <SDL3/SDL.h>
#include <utility>

namespace
{

constexpr std::pair<SDL_Scancode, unsigned int> KEYS[] = {
    { SDL_SCANCODE_RETURN, Key::RETURN },
    { SDL_SCANCODE_ESCAPE, Key::ESCAPE },
    { SDL_SCANCODE_BACKSPACE, Key::BACK },
    { SDL_SCANCODE_TAB, Key::TAB },
    { SDL_SCANCODE_SPACE, Key::SPACE },
    { SDL_SCANCODE_MINUS, Key::OEM_MINUS },
    { SDL_SCANCODE_EQUALS, Key::OEM_PLUS },
    { SDL_SCANCODE_LEFTBRACKET, Key::OEM_4 },
    { SDL_SCANCODE_RIGHTBRACKET, Key::OEM_6 },
    { SDL_SCANCODE_BACKSLASH, Key::OEM_5 },
    { SDL_SCANCODE_SEMICOLON, Key::OEM_1 },
    { SDL_SCANCODE_APOSTROPHE, Key::OEM_7 },
    { SDL_SCANCODE_GRAVE, Key::OEM_3 },
    { SDL_SCANCODE_COMMA, Key::OEM_COMMA },
    { SDL_SCANCODE_PERIOD, Key::OEM_PERIOD },
    { SDL_SCANCODE_SLASH, Key::OEM_2 },
    { SDL_SCANCODE_NONUSBACKSLASH, Key::OEM_102 },
    { SDL_SCANCODE_CAPSLOCK, Key::CAPITAL },
    { SDL_SCANCODE_F1, Key::F1 },
    { SDL_SCANCODE_F2, Key::F2 },
    { SDL_SCANCODE_F3, Key::F3 },
    { SDL_SCANCODE_F4, Key::F4 },
    { SDL_SCANCODE_F5, Key::F5 },
    { SDL_SCANCODE_F6, Key::F6 },
    { SDL_SCANCODE_F7, Key::F7 },
    { SDL_SCANCODE_F8, Key::F8 },
    { SDL_SCANCODE_F9, Key::F9 },
    { SDL_SCANCODE_F11, Key::F11 },
    { SDL_SCANCODE_F12, Key::F12 },
    { SDL_SCANCODE_PRINTSCREEN, Key::SNAPSHOT },
    { SDL_SCANCODE_SCROLLLOCK, Key::SCROLL },
    { SDL_SCANCODE_PAUSE, Key::PAUSE },
    { SDL_SCANCODE_INSERT, Key::INSERT },
    { SDL_SCANCODE_HOME, Key::HOME },
    { SDL_SCANCODE_PAGEUP, Key::PRIOR },
    { SDL_SCANCODE_DELETE, Key::DEL },
    { SDL_SCANCODE_END, Key::END },
    { SDL_SCANCODE_PAGEDOWN, Key::NEXT },
    { SDL_SCANCODE_RIGHT, Key::RIGHT },
    { SDL_SCANCODE_LEFT, Key::LEFT },
    { SDL_SCANCODE_DOWN, Key::DOWN },
    { SDL_SCANCODE_UP, Key::UP },
    { SDL_SCANCODE_NUMLOCKCLEAR, Key::NUMLOCK },
    { SDL_SCANCODE_KP_DIVIDE, Key::DIVIDE },
    { SDL_SCANCODE_KP_MULTIPLY, Key::MULTIPLY },
    { SDL_SCANCODE_KP_MINUS, Key::SUBTRACT },
    { SDL_SCANCODE_KP_PLUS, Key::ADD },
    { SDL_SCANCODE_KP_ENTER, Key::RETURN },
    { SDL_SCANCODE_KP_1, Key::NUMPAD1 },
    { SDL_SCANCODE_KP_2, Key::NUMPAD2 },
    { SDL_SCANCODE_KP_3, Key::NUMPAD3 },
    { SDL_SCANCODE_KP_4, Key::NUMPAD4 },
    { SDL_SCANCODE_KP_5, Key::NUMPAD5 },
    { SDL_SCANCODE_KP_6, Key::NUMPAD6 },
    { SDL_SCANCODE_KP_7, Key::NUMPAD7 },
    { SDL_SCANCODE_KP_8, Key::NUMPAD8 },
    { SDL_SCANCODE_KP_9, Key::NUMPAD9 },
    { SDL_SCANCODE_KP_0, Key::NUMPAD0 },
    { SDL_SCANCODE_KP_PERIOD, Key::DECIMAL },
    { SDL_SCANCODE_APPLICATION, Key::APPS },
    { SDL_SCANCODE_LCTRL, Key::CONTROL },
    { SDL_SCANCODE_RCTRL, Key::CONTROL },
    { SDL_SCANCODE_LSHIFT, Key::SHIFT },
    { SDL_SCANCODE_RSHIFT, Key::SHIFT },
    { SDL_SCANCODE_LGUI, Key::LWIN },
    { SDL_SCANCODE_RGUI, Key::RWIN },
};

} // namespace

const char* KeyMap::Name(unsigned int nKey)
{
    // A match reads a mouse button's flag word as a key.
    if (nKey == Key::LBUTTON)
    {
        return "Left mouse";
    }

    if (nKey == Key::RBUTTON)
    {
        return "Right mouse";
    }

    if ((nKey >= 'A' && nKey <= 'Z') || (nKey >= '0' && nKey <= '9'))
    {
        static char s_szName[2];

        s_szName[0] = (char)nKey;
        s_szName[1] = 0;

        return s_szName;
    }

    for (const auto& [scancode, nMapped] : KEYS)
    {
        if (nMapped == nKey)
        {
            return SDL_GetScancodeName(scancode);
        }
    }

    return "?";
}

unsigned int KeyMap::ToVirtualKey(int nScancode, int nKeycode)
{
    if (nKeycode >= (int)SDLK_A && nKeycode <= (int)SDLK_Z)
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
