#include "common/compat.h"
#include "input/keycodes.h"
#include "input/keymap.h"
#include <SDL3/SDL.h>
#include <utility>

namespace
{

constexpr std::pair<SDL_Scancode, unsigned int> KEYS[] = {
    { SDL_SCANCODE_RETURN, Key::kReturn },
    { SDL_SCANCODE_ESCAPE, Key::kEscape },
    { SDL_SCANCODE_BACKSPACE, Key::kBack },
    { SDL_SCANCODE_TAB, Key::kTab },
    { SDL_SCANCODE_SPACE, Key::kSpace },
    { SDL_SCANCODE_MINUS, Key::kOEMMinus },
    { SDL_SCANCODE_EQUALS, Key::kOEMPlus },
    { SDL_SCANCODE_LEFTBRACKET, Key::kOEM4 },
    { SDL_SCANCODE_RIGHTBRACKET, Key::kOEM6 },
    { SDL_SCANCODE_BACKSLASH, Key::kOEM5 },
    { SDL_SCANCODE_SEMICOLON, Key::kOEM1 },
    { SDL_SCANCODE_APOSTROPHE, Key::kOEM7 },
    { SDL_SCANCODE_GRAVE, Key::kOEM3 },
    { SDL_SCANCODE_COMMA, Key::kOEMComma },
    { SDL_SCANCODE_PERIOD, Key::kOEMPeriod },
    { SDL_SCANCODE_SLASH, Key::kOEM2 },
    { SDL_SCANCODE_NONUSBACKSLASH, Key::kOEM102 },
    { SDL_SCANCODE_CAPSLOCK, Key::kCapital },
    { SDL_SCANCODE_F1, Key::kF1 },
    { SDL_SCANCODE_F2, Key::kF2 },
    { SDL_SCANCODE_F3, Key::kF3 },
    { SDL_SCANCODE_F4, Key::kF4 },
    { SDL_SCANCODE_F5, Key::kF5 },
    { SDL_SCANCODE_F6, Key::kF6 },
    { SDL_SCANCODE_F7, Key::kF7 },
    { SDL_SCANCODE_F8, Key::kF8 },
    { SDL_SCANCODE_F9, Key::kF9 },
    { SDL_SCANCODE_F11, Key::kF11 },
    { SDL_SCANCODE_F12, Key::kF12 },
    { SDL_SCANCODE_PRINTSCREEN, Key::kSnapshot },
    { SDL_SCANCODE_SCROLLLOCK, Key::kScroll },
    { SDL_SCANCODE_PAUSE, Key::kPause },
    { SDL_SCANCODE_INSERT, Key::kInsert },
    { SDL_SCANCODE_HOME, Key::kHome },
    { SDL_SCANCODE_PAGEUP, Key::kPrior },
    { SDL_SCANCODE_DELETE, Key::kDel },
    { SDL_SCANCODE_END, Key::kEnd },
    { SDL_SCANCODE_PAGEDOWN, Key::kNext },
    { SDL_SCANCODE_RIGHT, Key::kRight },
    { SDL_SCANCODE_LEFT, Key::kLeft },
    { SDL_SCANCODE_DOWN, Key::kDown },
    { SDL_SCANCODE_UP, Key::kUp },
    { SDL_SCANCODE_NUMLOCKCLEAR, Key::kNumLock },
    { SDL_SCANCODE_KP_DIVIDE, Key::kDivide },
    { SDL_SCANCODE_KP_MULTIPLY, Key::kMultiply },
    { SDL_SCANCODE_KP_MINUS, Key::kSubtract },
    { SDL_SCANCODE_KP_PLUS, Key::kAdd },
    { SDL_SCANCODE_KP_ENTER, Key::kReturn },
    { SDL_SCANCODE_KP_1, Key::kNumpad1 },
    { SDL_SCANCODE_KP_2, Key::kNumpad2 },
    { SDL_SCANCODE_KP_3, Key::kNumpad3 },
    { SDL_SCANCODE_KP_4, Key::kNumpad4 },
    { SDL_SCANCODE_KP_5, Key::kNumpad5 },
    { SDL_SCANCODE_KP_6, Key::kNumpad6 },
    { SDL_SCANCODE_KP_7, Key::kNumpad7 },
    { SDL_SCANCODE_KP_8, Key::kNumpad8 },
    { SDL_SCANCODE_KP_9, Key::kNumpad9 },
    { SDL_SCANCODE_KP_0, Key::kNumpad0 },
    { SDL_SCANCODE_KP_PERIOD, Key::kDecimal },
    { SDL_SCANCODE_APPLICATION, Key::kApps },
    { SDL_SCANCODE_LCTRL, Key::kControl },
    { SDL_SCANCODE_RCTRL, Key::kControl },
    { SDL_SCANCODE_LSHIFT, Key::kShift },
    { SDL_SCANCODE_RSHIFT, Key::kShift },
    { SDL_SCANCODE_LGUI, Key::kLWin },
    { SDL_SCANCODE_RGUI, Key::kRWin },
};

} // namespace

const char* KeyMap::Name(unsigned int nKey)
{
    // A match reads a mouse button's flag word as a key.
    if (nKey == Key::kLButton)
    {
        return "Left mouse";
    }

    if (nKey == Key::kRButton)
    {
        return "Right mouse";
    }

    if ((nKey >= 'A' && nKey <= 'Z') || (nKey >= '0' && nKey <= '9'))
    {
        static char s_szName[2];

        s_szName[0] = static_cast<char>(nKey);
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
    if (nKeycode >= static_cast<int>(SDLK_A) && nKeycode <= static_cast<int>(SDLK_Z))
    {
        return 'A' + (nKeycode - SDLK_A);
    }

    // SDL numbers the top row 1 to 9, then 0.
    if (nScancode >= SDL_SCANCODE_1 && nScancode <= SDL_SCANCODE_9)
    {
        return '1' + static_cast<unsigned int>(nScancode - SDL_SCANCODE_1);
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
