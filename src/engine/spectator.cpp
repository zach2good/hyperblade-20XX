#include "engine/spectator.h"
#include "app/gameapp.h"
#include "app/wndproc.h"
#include "engine/engineobject.h"
#include "engine/framepacing.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "physics/physicsbody.h"
#include "ui/debugmenu.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

namespace
{

// Units a second, and Shift's multiple. A tick's step stays under FramePacing's cut distance, so
// the flight is blended between ticks.
constexpr auto kMoveSpeed  = 300.0f;
constexpr auto kFastFactor = 4.0f;

// Radians a mouse pixel, and radians a second at full stick.
constexpr auto kMouseTurn = 0.004f;
constexpr auto kStickTurn = 2.5f;

// Just short of straight up and down, where the camera's basis would fold.
constexpr auto kPitchLimit = 1.5f;

constexpr auto kStickDeadZone = 8000.0f;
constexpr auto kStickRange    = 32767.0f;

bool s_bAutopilot  = false;
bool s_bFreeCamera = false;
bool s_bSeeded     = false;

Vec3  s_eye     = {};
float s_flYaw   = 0.0f;
float s_flPitch = 0.0f;

// Lets go of every key the player held, in the input device and on slot 0's dromer.
void ReleasePlayer()
{
    WndProc::ReleaseHeldKeys();

    if (g_pGameEngine == nullptr || g_gameApp.m_nEngineState != kEstateMatchLive)
    {
        return;
    }

    auto* const pPlayer = g_pGameEngine->m_ppSlots[0];

    if (pPlayer == nullptr)
    {
        return;
    }

    auto* const pBody        = pPlayer->GetPhysicsBody();
    pBody->nHeldBindings     = 0;
    pBody->nPrevHeldBindings = 0;
}

// A stick axis scaled to -1..1, with the dead zone taken out.
float Stick(SDL_Gamepad* pPad, SDL_GamepadAxis axis)
{
    const auto flRaw = static_cast<float>(SDL_GetGamepadAxis(pPad, axis));

    if (std::fabs(flRaw) < kStickDeadZone)
    {
        return 0.0f;
    }

    return std::clamp(flRaw / kStickRange, -1.0f, 1.0f);
}

} // namespace

bool Spectator::Autopilot()
{
    return s_bAutopilot;
}

void Spectator::SetAutopilot(bool bOn)
{
    if (bOn == s_bAutopilot || g_bSessionActive != 0)
    {
        return;
    }

    s_bAutopilot = bOn;

    ReleasePlayer();

    if (g_pGameEngine != nullptr && g_gameApp.m_nEngineState == kEstateMatchLive && g_pGameEngine->m_ppSlots[0] != nullptr)
    {
        g_pGameEngine->m_ppSlots[0]->SelectAndActivatePlan();
    }
}

bool Spectator::FreeCamera()
{
    return s_bFreeCamera;
}

void Spectator::SetFreeCamera(bool bOn)
{
    if (bOn == s_bFreeCamera)
    {
        return;
    }

    s_bFreeCamera = bOn;
    s_bSeeded     = false;

    ReleasePlayer();

    // Drains the motion gathered since the last flight.
    SDL_GetRelativeMouseState(nullptr, nullptr);
}

bool Spectator::HoldsInput()
{
    return s_bAutopilot || s_bFreeCamera;
}

std::optional<Spectator::View> Spectator::FreeView(const Vec3& eye, const Vec3& dir)
{
    if (!s_bFreeCamera)
    {
        return std::nullopt;
    }

    if (!s_bSeeded)
    {
        s_eye     = eye;
        s_flYaw   = std::atan2(dir.y, dir.x);
        s_flPitch = std::asin(std::clamp(dir.z, -1.0f, 1.0f));
        s_bSeeded = true;
    }

    auto flForward = 0.0f;
    auto flSide    = 0.0f;
    auto flRise    = 0.0f;
    auto flTurn    = 0.0f;
    auto flTilt    = 0.0f;
    auto bFast     = false;

    if (!DebugMenu::WantsKeyboard())
    {
        const auto* const pKeys = SDL_GetKeyboardState(nullptr);

        flForward += static_cast<float>(pKeys[SDL_SCANCODE_W]) - static_cast<float>(pKeys[SDL_SCANCODE_S]);
        flSide += static_cast<float>(pKeys[SDL_SCANCODE_D]) - static_cast<float>(pKeys[SDL_SCANCODE_A]);
        flRise += static_cast<float>(pKeys[SDL_SCANCODE_E]) - static_cast<float>(pKeys[SDL_SCANCODE_Q]);
        bFast = pKeys[SDL_SCANCODE_LSHIFT] || pKeys[SDL_SCANCODE_RSHIFT];
    }

    // Read every tick so motion made with the button up never lands as a turn later.
    auto       flMouseX = 0.0f;
    auto       flMouseY = 0.0f;
    const auto buttons  = SDL_GetRelativeMouseState(&flMouseX, &flMouseY);

    if ((buttons & SDL_BUTTON_RMASK) != 0 && !DebugMenu::WantsMouse())
    {
        s_flYaw += flMouseX * kMouseTurn;
        s_flPitch -= flMouseY * kMouseTurn;
    }

    auto nPads = 0;

    // The first pad the game opened flies too: left stick moves, right stick looks, triggers rise.
    if (auto* const pIds = SDL_GetGamepads(&nPads); pIds != nullptr)
    {
        auto* const pPad = [&]() -> SDL_Gamepad*
        {
            if (nPads == 0)
            {
                return nullptr;
            }

            return SDL_GetGamepadFromID(pIds[0]);
        }();

        if (pPad != nullptr)
        {
            flForward -= Stick(pPad, SDL_GAMEPAD_AXIS_LEFTY);
            flSide += Stick(pPad, SDL_GAMEPAD_AXIS_LEFTX);
            flTurn += Stick(pPad, SDL_GAMEPAD_AXIS_RIGHTX);
            flTilt -= Stick(pPad, SDL_GAMEPAD_AXIS_RIGHTY);
            flRise += Stick(pPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) - Stick(pPad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
            bFast = bFast || SDL_GetGamepadButton(pPad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        }

        SDL_free(pIds);
    }

    const auto flStep = FramePacing::kTickMs / 1000.0f;

    s_flYaw += flTurn * kStickTurn * flStep;
    s_flPitch = std::clamp(s_flPitch + flTilt * kStickTurn * flStep, -kPitchLimit, kPitchLimit);

    const auto flCosPitch = std::cos(s_flPitch);

    auto view = View{};
    view.dir  = Vec3{ flCosPitch * std::cos(s_flYaw), flCosPitch * std::sin(s_flYaw), std::sin(s_flPitch) };

    // The camera's right is world up crossed with its direction, as ViewIt builds it.
    const auto right = Vec3{ -std::sin(s_flYaw), std::cos(s_flYaw), 0.0f };

    const auto flSpeed = [&]
    {
        if (bFast)
        {
            return kMoveSpeed * kFastFactor;
        }

        return kMoveSpeed;
    }();

    const auto flDistance = flSpeed * flStep;

    s_eye.x += (view.dir.x * flForward + right.x * flSide) * flDistance;
    s_eye.y += (view.dir.y * flForward + right.y * flSide) * flDistance;
    s_eye.z += (view.dir.z * flForward + flRise) * flDistance;

    view.eye = s_eye;

    return view;
}
