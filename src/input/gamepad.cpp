#include "common/compat.h"
#include "input/keycodes.h"
#include "gamepad.h"
#include "globals.h"
#include "app/gameapp.h"
#include "app/wndproc.h"
#include "engine/gameengine.h"
#include "engine/gameoptions.h"
#include "input/inputglobals.h"
#include "input/inputlog.h"
#include "input/keyboarddeviceobj.h"
#include <SDL3/SDL.h>
#include <array>

namespace
{

// A stick past half its travel presses that way, as the original's joystick did.
constexpr auto STICK_THRESHOLD = 16384;

// A trigger presses past half its travel and lets go below a quarter, so a light hold never chatters.
constexpr auto TRIGGER_PRESS   = 16384;
constexpr auto TRIGGER_RELEASE = 8192;

// In the order of BINDING_UP to BINDING_RIGHT.
enum Direction
{
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_COUNT
};

// What a pad control presses in a match: one of the key bindings, as the keyboard has it, or a key
// of its own. The names are the ones booie.ini [Gamepad] uses.
struct Action
{
    const char*  pszName;
    int          nBinding;
    unsigned int nKey;
};

constexpr Action ACTIONS[] = {
    { "None", -1, 0 },
    { "Attack", 4, 0 },
    { "SpeedBurst", 5, 0 },
    { "Shoot", 6, 0 },
    { "Pass", 7, 0 },
    { "QuickShot", 8, 0 },
    { "Jump", 9, 0 },
    { "Substitute1", -1, '1' },
    { "Substitute2", -1, '2' },
    { "Camera", -1, Key::TAB },
    // Escape pauses a match.
    { "Pause", -1, Key::ESCAPE },
};

// The controls booie.ini [Gamepad] can bind, with their defaults and what each presses in the
// menus. The menus move with left and right only, so up goes back and down goes on.
struct Control
{
    const char*  pszKey;
    const char*  pszDefault;
    unsigned int nMenuKey;
};

enum ControlId
{
    CONTROL_A,
    CONTROL_B,
    CONTROL_X,
    CONTROL_Y,
    CONTROL_LEFT_SHOULDER,
    CONTROL_RIGHT_SHOULDER,
    CONTROL_LEFT_TRIGGER,
    CONTROL_RIGHT_TRIGGER,
    CONTROL_DPAD_UP,
    CONTROL_DPAD_DOWN,
    CONTROL_DPAD_LEFT,
    CONTROL_DPAD_RIGHT,
    CONTROL_BACK,
    CONTROL_START,
    CONTROL_COUNT
};

constexpr std::array<Control, CONTROL_COUNT> CONTROLS = { {
    { "A", "Attack", Key::RETURN },
    { "B", "Jump", Key::ESCAPE },
    { "X", "SpeedBurst", 0 },
    { "Y", "Pass", 0 },
    { "LeftShoulder", "None", 0 },
    { "RightShoulder", "QuickShot", 0 },
    { "LeftTrigger", "None", 0 },
    { "RightTrigger", "Shoot", 0 },
    { "DPadUp", "None", Key::LEFT },
    { "DPadDown", "None", Key::RIGHT },
    { "DPadLeft", "Substitute1", Key::LEFT },
    { "DPadRight", "Substitute2", Key::RIGHT },
    { "Back", "Camera", 0 },
    { "Start", "Pause", Key::ESCAPE },
} };

// A held control's key, so its release reaches the same key, and what pressed it, for the log.
struct Press
{
    bool         bDown     = false;
    unsigned int nKey      = 0;
    const char*  pszSource = nullptr;
    const char*  pszAction = nullptr;
};

std::array<Press, DIRECTION_COUNT>       s_aDirections;
std::array<Press, CONTROL_COUNT>         s_aControls;
std::array<bool, DIRECTION_COUNT>        s_abStick         = {};
std::array<bool, 2>                      s_abTrigger       = {};
std::array<const Action*, CONTROL_COUNT> s_apBindings      = {};
bool                                     s_bBindingsLoaded = false;
SDL_JoystickID                           s_nLastPad        = 0;

bool InMatch()
{
    return g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE && g_pGameEngine != nullptr && g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING && g_pInputDevice != nullptr;
}

const Action& FindAction(const std::string& name)
{
    for (const auto& action : ACTIONS)
    {
        if (SDL_strcasecmp(action.pszName, name.c_str()) == 0)
        {
            return action;
        }
    }

    return ACTIONS[0];
}

void LoadBindings()
{
    if (s_bBindingsLoaded || g_pGameOptions == nullptr)
    {
        return;
    }

    s_bBindingsLoaded = true;

    for (size_t i = 0; i < CONTROLS.size(); i++)
    {
        s_apBindings[i] = &FindAction(g_pGameOptions->ReadString("Gamepad", CONTROLS[i].pszKey, CONTROLS[i].pszDefault));
    }
}

// The key an action presses now, by the keyboard's current bindings.
unsigned int ActionKey(const Action& action)
{
    if (action.nBinding >= 0)
    {
        return g_pInputDevice->GetDefaultKeyBindings()[action.nBinding];
    }

    return action.nKey;
}

void Down(Press& press, unsigned int nKey, const char* pszSource, const char* pszAction)
{
    if (press.bDown || nKey == 0)
    {
        return;
    }

    press = Press{ true, nKey, pszSource, pszAction };

    InputLog::Add(pszSource, nKey, true, pszAction);
    WndProc::OnKeyDown(nKey);
}

void Up(Press& press)
{
    if (press.bDown)
    {
        InputLog::Add(press.pszSource, press.nKey, false, press.pszAction);
        WndProc::OnKeyUp(press.nKey);
    }

    press = Press{};
}

// The left stick steers in a match and moves through the menus.
void UpdateDirections()
{
    constexpr std::array<unsigned int, DIRECTION_COUNT> MENU_KEYS        = { Key::LEFT, Key::RIGHT, Key::LEFT, Key::RIGHT };
    constexpr std::array<const char*, DIRECTION_COUNT> NAMES             = { "Stick up", "Stick down", "Stick left", "Stick right" };
    constexpr std::array<const char*, DIRECTION_COUNT> DIRECTION_ACTIONS = { "Forward", "Stop", "RotateLeft", "RotateRight" };

    for (int nDirection = 0; nDirection < DIRECTION_COUNT; nDirection++)
    {
        if (!s_abStick[nDirection])
        {
            Up(s_aDirections[nDirection]);

            continue;
        }

        const auto nKey = [&]
        {
            if (InMatch())
            {
                return g_pInputDevice->GetDefaultKeyBindings()[nDirection];
            }

            return MENU_KEYS[nDirection];
        }();

        const auto* const pszAction = [&]() -> const char*
        {
            if (InMatch())
            {
                return DIRECTION_ACTIONS[nDirection];
            }

            return nullptr;
        }();

        Down(s_aDirections[nDirection], nKey, NAMES[nDirection], pszAction);
    }
}

void OnControl(ControlId control, bool bDown)
{
    auto& press = s_aControls[control];

    if (!bDown)
    {
        Up(press);

        return;
    }

    LoadBindings();

    const auto bInMatch = InMatch();

    const auto nKey = [&]
    {
        if (bInMatch)
        {
            return ActionKey(*s_apBindings[control]);
        }

        return CONTROLS[control].nMenuKey;
    }();

    const auto* const pszAction = [&]() -> const char*
    {
        if (bInMatch)
        {
            return s_apBindings[control]->pszName;
        }

        return nullptr;
    }();

    Down(press, nKey, CONTROLS[control].pszKey, pszAction);
}

void OnButton(SDL_GamepadButton button, bool bDown)
{
    constexpr std::pair<SDL_GamepadButton, ControlId> BUTTONS[] = {
        { SDL_GAMEPAD_BUTTON_SOUTH, CONTROL_A },
        { SDL_GAMEPAD_BUTTON_EAST, CONTROL_B },
        { SDL_GAMEPAD_BUTTON_WEST, CONTROL_X },
        { SDL_GAMEPAD_BUTTON_NORTH, CONTROL_Y },
        { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, CONTROL_LEFT_SHOULDER },
        { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, CONTROL_RIGHT_SHOULDER },
        { SDL_GAMEPAD_BUTTON_DPAD_UP, CONTROL_DPAD_UP },
        { SDL_GAMEPAD_BUTTON_DPAD_DOWN, CONTROL_DPAD_DOWN },
        { SDL_GAMEPAD_BUTTON_DPAD_LEFT, CONTROL_DPAD_LEFT },
        { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, CONTROL_DPAD_RIGHT },
        { SDL_GAMEPAD_BUTTON_BACK, CONTROL_BACK },
        { SDL_GAMEPAD_BUTTON_START, CONTROL_START },
    };

    for (const auto& [mapped, control] : BUTTONS)
    {
        if (mapped == button)
        {
            OnControl(control, bDown);

            return;
        }
    }
}

void OnTrigger(int nTrigger, ControlId control, int nValue)
{
    auto& bHeld = s_abTrigger[(size_t)nTrigger];

    const auto bNowHeld = [&]
    {
        if (bHeld)
        {
            return nValue > TRIGGER_RELEASE;
        }

        return nValue > TRIGGER_PRESS;
    }();

    if (bNowHeld != bHeld)
    {
        bHeld = bNowHeld;

        OnControl(control, bHeld);
    }
}

void OnAxis(SDL_GamepadAxis axis, int nValue)
{
    switch (axis)
    {
        case SDL_GAMEPAD_AXIS_LEFTX:
            s_abStick[DIRECTION_LEFT]  = nValue < -STICK_THRESHOLD;
            s_abStick[DIRECTION_RIGHT] = nValue > STICK_THRESHOLD;
            UpdateDirections();
            break;

        case SDL_GAMEPAD_AXIS_LEFTY:
            s_abStick[DIRECTION_UP]   = nValue < -STICK_THRESHOLD;
            s_abStick[DIRECTION_DOWN] = nValue > STICK_THRESHOLD;
            UpdateDirections();
            break;

        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
            OnTrigger(0, CONTROL_LEFT_TRIGGER, nValue);
            break;

        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
            OnTrigger(1, CONTROL_RIGHT_TRIGGER, nValue);
            break;

        default:
            break;
    }
}

void ReleaseAll()
{
    s_abStick   = {};
    s_abTrigger = {};

    UpdateDirections();

    for (auto& press : s_aControls)
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
            s_nLastPad = pEvent->gbutton.which;
            OnButton((SDL_GamepadButton)pEvent->gbutton.button, pEvent->gbutton.down);
            break;

        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            // A resting stick drifts, so only a real push counts as using the pad.
            if (pEvent->gaxis.value > TRIGGER_RELEASE || pEvent->gaxis.value < -TRIGGER_RELEASE)
            {
                s_nLastPad = pEvent->gaxis.which;
            }

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

int Gamepad::BindingCount()
{
    return (int)CONTROLS.size();
}

Gamepad::Binding Gamepad::GetBinding(int nIndex)
{
    LoadBindings();

    const auto* const pszAction = [&]
    {
        if (s_apBindings[(size_t)nIndex] != nullptr)
        {
            return s_apBindings[(size_t)nIndex]->pszName;
        }

        return CONTROLS[(size_t)nIndex].pszDefault;
    }();

    return Binding{ CONTROLS[(size_t)nIndex].pszKey, pszAction };
}

unsigned int Gamepad::LastUsed()
{
    return s_nLastPad;
}

void Gamepad::Shutdown()
{
    SDL_RemoveEventWatch(OnEvent, nullptr);
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}
