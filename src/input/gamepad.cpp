#include "common/compat.h"
#include "gamepad.h"
#include "globals.h"
#include "app/gameapp.h"
#include "app/wndproc.h"
#include "engine/gameengine.h"
#include "engine/gameoptions.h"
#include "input/inputglobals.h"
#include "input/keyboarddeviceobj.h"
#include <SDL3/SDL.h>
#include <array>

namespace
{

// A stick past half its travel presses that way, as the original's joystick did.
constexpr auto STICK_THRESHOLD = 16384;

// In the order of BINDING_UP to BINDING_RIGHT.
enum Direction
{
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_COUNT
};

// The four action buttons, in the original's button order: attack, speed burst, shoot, pass.
constexpr std::array<SDL_GamepadButton, 4> s_aActionButtons = {
    SDL_GAMEPAD_BUTTON_SOUTH,
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
    SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH,
};

// booie.ini [Gamepad] names the action each button presses.
constexpr std::array<const char*, 4> s_apszButtonKeys = { "Attack", "Speed", "Shoot", "Pass" };

enum class Route
{
    NONE,
    MATCH,
    MENU
};

// Where a press went, so its release goes to the same place.
struct Press
{
    Route        route = Route::NONE;
    unsigned int nKey  = 0;
};

std::array<Press, DIRECTION_COUNT>          s_aDirections;
std::array<Press, SDL_GAMEPAD_BUTTON_COUNT> s_aButtons;
std::array<bool, DIRECTION_COUNT>           s_abStick = {};
std::array<bool, DIRECTION_COUNT>           s_abPad   = {};
std::array<int, 4>                          s_anButtonBindings;
bool                                        s_bBindingsLoaded = false;

bool InMatch()
{
    return g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE && g_pGameEngine != 0 && g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING && g_pInputDevice != 0;
}

// Each button defaults to its own action; a value outside one to four leaves it unbound.
void LoadButtonBindings()
{
    if (s_bBindingsLoaded || g_pGameOptions == 0)
    {
        return;
    }

    s_bBindingsLoaded = true;

    for (int i = 0; i < (int)s_anButtonBindings.size(); i++)
    {
        const int nButton = (int)g_pGameOptions->ReadInt("Gamepad", s_apszButtonKeys[i], i + 1) - 1;

        s_anButtonBindings[i] = [&]
        {
            if (nButton >= 0 && nButton < 4)
            {
                return nButton + BINDING_ATTACK;
            }

            return -1;
        }();
    }
}

void Down(Press& press, int nBinding, unsigned int nMenuKey)
{
    if (press.route != Route::NONE)
    {
        return;
    }

    if (nBinding >= 0 && InMatch())
    {
        press = Press{ Route::MATCH, g_pInputDevice->GetDefaultKeyBindings()[nBinding] };

        g_pInputDevice->HandleKeyDown(press.nKey);

        return;
    }

    if (nMenuKey == 0)
    {
        return;
    }

    press = Press{ Route::MENU, nMenuKey };

    WndProc::OnKeyDown(nMenuKey);
}

void Up(Press& press)
{
    if (press.route == Route::MATCH && g_pInputDevice != 0)
    {
        g_pInputDevice->HandleKeyUp(press.nKey);
    }
    else if (press.route == Route::MENU)
    {
        WndProc::OnKeyUp(press.nKey);
    }

    press = Press{};
}

// The menus move with left and right only, so up goes back and down goes on.
void UpdateDirections()
{
    constexpr std::array<unsigned int, DIRECTION_COUNT> s_anMenuKeys = { VK_LEFT, VK_RIGHT, VK_LEFT, VK_RIGHT };

    for (int nDirection = 0; nDirection < DIRECTION_COUNT; nDirection++)
    {
        const bool bHeld = s_abStick[nDirection] || s_abPad[nDirection];

        if (bHeld)
        {
            Down(s_aDirections[nDirection], nDirection, s_anMenuKeys[nDirection]);
        }
        else if (s_aDirections[nDirection].route != Route::NONE)
        {
            Up(s_aDirections[nDirection]);
        }
    }
}

void OnButton(SDL_GamepadButton button, bool bDown)
{
    switch (button)
    {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            s_abPad[DIRECTION_UP] = bDown;
            UpdateDirections();
            return;

        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            s_abPad[DIRECTION_DOWN] = bDown;
            UpdateDirections();
            return;

        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            s_abPad[DIRECTION_LEFT] = bDown;
            UpdateDirections();
            return;

        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            s_abPad[DIRECTION_RIGHT] = bDown;
            UpdateDirections();
            return;

        default:
            break;
    }

    if (button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT)
    {
        return;
    }

    auto& press = s_aButtons[button];

    if (!bDown)
    {
        Up(press);

        return;
    }

    LoadButtonBindings();

    const auto nBinding = [&]
    {
        for (int i = 0; i < (int)s_aActionButtons.size(); i++)
        {
            if (s_aActionButtons[i] == button)
            {
                return s_anButtonBindings[i];
            }
        }

        return -1;
    }();

    // Start pauses in a match, as Escape does.
    const auto nMenuKey = [&]() -> unsigned int
    {
        if (button == SDL_GAMEPAD_BUTTON_SOUTH)
        {
            return VK_RETURN;
        }

        if (button == SDL_GAMEPAD_BUTTON_EAST || button == SDL_GAMEPAD_BUTTON_START)
        {
            return VK_ESCAPE;
        }

        return 0;
    }();

    Down(press, nBinding, nMenuKey);
}

void OnAxis(SDL_GamepadAxis axis, int nValue)
{
    if (axis == SDL_GAMEPAD_AXIS_LEFTX)
    {
        s_abStick[DIRECTION_LEFT]  = nValue < -STICK_THRESHOLD;
        s_abStick[DIRECTION_RIGHT] = nValue > STICK_THRESHOLD;
    }
    else if (axis == SDL_GAMEPAD_AXIS_LEFTY)
    {
        s_abStick[DIRECTION_UP]   = nValue < -STICK_THRESHOLD;
        s_abStick[DIRECTION_DOWN] = nValue > STICK_THRESHOLD;
    }
    else
    {
        return;
    }

    UpdateDirections();
}

void ReleaseAll()
{
    s_abStick = {};
    s_abPad   = {};

    UpdateDirections();

    for (auto& press : s_aButtons)
    {
        Up(press);
    }
}

bool SDLCALL OnEvent(void*, SDL_Event* pEvent)
{
    switch (pEvent->type)
    {
        case SDL_EVENT_GAMEPAD_ADDED:
            SDL_OpenGamepad(pEvent->gdevice.which);
            break;

        case SDL_EVENT_GAMEPAD_REMOVED:
            SDL_CloseGamepad(SDL_GetGamepadFromID(pEvent->gdevice.which));
            ReleaseAll();
            break;

        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            OnButton((SDL_GamepadButton)pEvent->gbutton.button, pEvent->gbutton.down);
            break;

        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            OnAxis((SDL_GamepadAxis)pEvent->gaxis.axis, pEvent->gaxis.value);
            break;

        default:
            break;
    }

    return true;
}

} // namespace

void Gamepad::Init()
{
    // Pads already plugged in are announced during the init, so the watch goes first.
    SDL_AddEventWatch(OnEvent, nullptr);

    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
    {
        SDL_RemoveEventWatch(OnEvent, nullptr);
    }
}

void Gamepad::Shutdown()
{
    SDL_RemoveEventWatch(OnEvent, nullptr);
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}
