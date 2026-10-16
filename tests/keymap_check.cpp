// Maps SDL keys to the virtual-key codes the key bindings store.

#include "common/compat.h"
#include "input/keymap.h"
#include <SDL3/SDL.h>
#include <cstdio>

static int s_nFailed = 0;

static void Expect(unsigned int nGot, unsigned int nWanted, const char* pszWhat)
{
    if (nGot != nWanted)
    {
        std::printf("failed: %s, got 0x%x, wanted 0x%x\n", pszWhat, nGot, nWanted);
        s_nFailed++;
    }
}

int main(int, char**)
{
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_A, SDLK_A), 'A', "letter");

    // An AZERTY A sits where QWERTY's Q is, and virtual keys follow the letter.
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_Q, SDLK_A), 'A', "letter follows the layout");

    // AZERTY's top row types & but is still VK 1.
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_1, SDLK_AMPERSAND), '1', "digit follows the position");
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_0, SDLK_0), '0', "zero");

    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_F2, SDLK_F2), VK_F2, "function key");
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_KP_ENTER, SDLK_KP_ENTER), VK_RETURN, "keypad enter is return");
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_RSHIFT, SDLK_RSHIFT), VK_SHIFT, "either shift is shift");
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_EQUALS, SDLK_EQUALS), VK_OEM_PLUS, "equals is OEM plus");

    // Windows sent these as system keys, which the game never saw.
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_LALT, SDLK_LALT), 0, "alt");
    Expect(KeyMap::ToVirtualKey(SDL_SCANCODE_F10, SDLK_F10), 0, "F10");

    std::printf("%d failed\n", s_nFailed);

    if (s_nFailed != 0)
    {
        return 1;
    }

    return 0;
}
