#include "common/compat.h"
#include "engine/spectator.h"
#include "common/clock.h"
#include "common/sdlmemory.h"
#include "ui/debugmenu.h"
#include "ui/originalui.h"
#include "ui/scenes.h"
#include "app/config.h"
#include "app/gameapp.h"
#include "app/window.h"
#include "engine/engineobject.h"
#include "engine/framepacing.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/planfixes.h"
#include "globals.h"
#include "graphics/graphicssettings.h"
#include "graphics/meshblock.h"
#include "graphics/meshbone.h"
#include "graphics/presenter.h"
#include "graphics/renderer.h"
#include "input/cheat.h"
#include "input/cheattable.h"
#include "input/gamepad.h"
#include "input/inputglobals.h"
#include "input/inputlog.h"
#include "input/keyboarddeviceobj.h"
#include "input/keymap.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <string>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

namespace
{

using GraphicsSettings::Switch;

bool s_bReady      = false;
bool s_bOpen       = false;
bool s_bShowHidden = true;

// The original's menu bar hides like a full-screen menu bar: it is up while the debug menu is, and
// otherwise while the pointer is at the top of the window or a menu hangs from the bar.
bool  s_bMenuBarKept    = false;
float s_flMenuBarHeight = 20.0f;

bool PointerAtTop()
{
    if (SDL_GetMouseFocus() == nullptr)
    {
        return false;
    }

    auto flY = 0.0f;

    SDL_GetMouseState(nullptr, &flY);

    return flY < s_flMenuBarHeight;
}

bool MenuBarShown()
{
    return s_bOpen || s_bMenuBarKept || PointerAtTop();
}

// An original dialog stays up after the debug menu closes, until its own buttons close it.
bool AnythingShown()
{
    return MenuBarShown() || OriginalUI::HasOpenDialogs() || Scenes::Current() != Scenes::Scene::None;
}

void Ready(SDL_Renderer* pRenderer)
{
    if (s_bReady)
    {
        return;
    }

    ImGui::CreateContext();

    auto& io = ImGui::GetIO();

    io.IniFilename = nullptr;

    // The game sets the cursor itself.
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui::StyleColorsDark();

    // A little larger than ImGui's 13 pixels, in its scalable default font so it stays sharp.
    ImGui::GetStyle().FontSizeBase = 16.0f;
    io.Fonts->AddFontDefaultVector();

    ImGui_ImplSDL3_InitForSDLRenderer(Window::Native().As<SDL_Window>(), pRenderer);
    ImGui_ImplSDLRenderer3_Init(pRenderer);

    s_bReady = true;
}

void Checkbox(const char* pszLabel, Switch which, const char* pszTooltip)
{
    auto bOn = GraphicsSettings::On(which);

    if (OriginalUI::Checkbox(pszLabel, &bOn))
    {
        GraphicsSettings::Toggle(which);
    }

    OriginalUI::SetItemTooltip("%s", pszTooltip);
}

bool Hardware()
{
    return GraphicsSettings::Active() == GraphicsSettings::Renderer::HARDWARE;
}

// The hardware renderer's switches do nothing in software, so they are shown greyed out.
void BeginHardwareOnly()
{
    if (!Hardware())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.0f, 0.0f, 1.0f), "The software renderer draws the original look,");
        ImGui::TextColored(ImVec4(0.5f, 0.0f, 0.0f, 1.0f), "so these only take effect on the hardware renderer.");

        ImGui::BeginDisabled(!Presenter::HasGPU());

        if (OriginalUI::Button("Switch to hardware"))
        {
            GraphicsSettings::ToggleRenderer();
        }

        ImGui::EndDisabled();
        ImGui::Spacing();
    }

    ImGui::BeginDisabled(!Hardware());
}

void GraphicsTab()
{
    OriginalUI::SeparatorText("Renderer");

    ImGui::BeginDisabled(!Presenter::HasGPU());

    if (OriginalUI::RadioButton("Original software renderer", !Hardware()) && Hardware())
    {
        GraphicsSettings::ToggleRenderer();
    }

    ImGui::SameLine();

    if (OriginalUI::RadioButton("New hardware renderer", Hardware()) && !Hardware())
    {
        GraphicsSettings::ToggleRenderer();
    }

    ImGui::EndDisabled();

    if (!Presenter::HasGPU())
    {
        ImGui::TextDisabled("No GPU device, so software only.");
    }

    BeginHardwareOnly();

    OriginalUI::SeparatorText("Models");
    Checkbox("Textured models", Switch::TEXTURES, "The textured model set, as the original's Direct3D mode drew it. Off draws the shaded models in palette colours.");
    Checkbox("Full detail at any distance", Switch::FULL_DETAIL, "Always the most detailed textured model. Off steps down with distance, as the engine picks.");

    auto bDrome = g_nDromeTextured != 0;

    ImGui::BeginDisabled(g_bShadedDromeModels != 0);

    if (OriginalUI::Checkbox("Textured drome (F8)", &bDrome))
    {
        Renderer::ToggleDromeTextured();
    }

    OriginalUI::SetItemTooltip("The original's F8: the drome with its textures, or shaded. The -m switch keeps it shaded.");
    ImGui::EndDisabled();

    OriginalUI::SeparatorText("Surfaces");
    Checkbox("Smooth texture filtering", Switch::LINEAR_FILTERING, "Linear filtering, as the original's F7 toggled. Off is nearest.");

    OriginalUI::SeparatorText("Shading");

    const auto bSmooth = GraphicsSettings::On(Switch::SMOOTH_SHADING);
    const auto bPhong  = GraphicsSettings::On(Switch::PHONG_SHADING);

    if (OriginalUI::RadioButton("Flat (original)", !bSmooth))
    {
        if (bSmooth)
        {
            GraphicsSettings::Toggle(Switch::SMOOTH_SHADING);
        }
    }

    OriginalUI::SetItemTooltip("Each face lit as one, as the original did.");
    ImGui::SameLine();

    if (OriginalUI::RadioButton("Gouraud shading", bSmooth && !bPhong))
    {
        if (!bSmooth)
        {
            GraphicsSettings::Toggle(Switch::SMOOTH_SHADING);
        }

        if (bPhong)
        {
            GraphicsSettings::Toggle(Switch::PHONG_SHADING);
        }
    }

    OriginalUI::SetItemTooltip("Light worked out at each corner and blended across the face, smooth across edges that don't crease.");
    ImGui::SameLine();

    if (OriginalUI::RadioButton("Phong shading", bSmooth && bPhong))
    {
        if (!bSmooth)
        {
            GraphicsSettings::Toggle(Switch::SMOOTH_SHADING);
        }

        if (!bPhong)
        {
            GraphicsSettings::Toggle(Switch::PHONG_SHADING);
        }
    }

    OriginalUI::SetItemTooltip("Light worked out at each pixel from the blended normal, for the smoothest highlights.");

    OriginalUI::SeparatorText("Draw order");
    Checkbox("Paint in original order", Switch::PAINTERS_ORDER, "Each mesh covers the ones sorted behind it, as the original painted them. Off depth tests every mesh against the others.");

    ImGui::EndDisabled();
}

void LightingTab()
{
    BeginHardwareOnly();

    OriginalUI::SeparatorText("Scene lights");
    Checkbox("Light from 303\xc2\xb0", Switch::LIGHT_303, "One of the two lights above the arena.");
    Checkbox("Light from 54\xc2\xb0", Switch::LIGHT_54, "The other light above the arena.");
    Checkbox("Light from below", Switch::LIGHT_180, "Lights the drome's first piece from underneath.");
    Checkbox("Fill from every light", Switch::SEVERAL_LIGHTS, "The original lights each face by one light. On, the other lights fill in where they face it.");

    OriginalUI::SeparatorText("Rok");
    Checkbox("Rok emits light", Switch::ROK_LIGHT, "A point light carried by the rok, added over the scene lights.");

    ImGui::BeginDisabled(!GraphicsSettings::On(Switch::ROK_LIGHT));

    auto flRadius = GraphicsSettings::RokLightRadius();

    if (ImGui::SliderFloat("Reach", &flRadius, 10.0f, 2000.0f, "%.0f", ImGuiSliderFlags_Logarithmic))
    {
        GraphicsSettings::SetRokLightRadius(flRadius);
    }

    OriginalUI::SetItemTooltip("How far the glow reaches, in world units. Not saved.");
    ImGui::EndDisabled();

    OriginalUI::SeparatorText("Shadows");
    Checkbox("Realtime shadows", Switch::SHADOW_MAPS, "Objects cast shadow maps from both lights above the arena, in place of the original's shadow shapes.");

    ImGui::BeginDisabled(!GraphicsSettings::On(Switch::SHADOW_MAPS));

    auto flSoftness = GraphicsSettings::ShadowSoftness();

    if (ImGui::SliderFloat("Softness", &flSoftness, 0.0f, 1.5f, "%.2f"))
    {
        GraphicsSettings::SetShadowSoftness(flSoftness);
    }

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        GraphicsSettings::SaveShadowSoftness();
    }

    OriginalUI::SetItemTooltip("How far a shadow's edge spreads, in world units. Nought is a hard edge.");
    ImGui::EndDisabled();

    ImGui::EndDisabled();
}

void TimingTab()
{
    OriginalUI::SeparatorText("Frame rate");
    Checkbox("Uncapped", Switch::UNCAPPED, "A match draws as often as it can, blending between the 30 Hz simulation ticks. Off draws once a tick.");

    OriginalUI::SeparatorText("Simulation");
    ImGui::Text("%d ticks a second", FramePacing::TicksPerSecond());
    OriginalUI::SetItemTooltip("Should read 30 in a match.");
    ImGui::Text("Clock drift %+.0f ms a second", FramePacing::DriftMs());
    OriginalUI::SetItemTooltip("How far the ticks ran ahead of the wall clock. Should read near 0.");
}

// The key bindings in use: the input device's once a match has one, the saved ones before.
const unsigned int* KeyBindings()
{
    if (g_pInputDevice != nullptr)
    {
        return g_pInputDevice->GetDefaultKeyBindings();
    }

    return g_anKeyBindingDefaults;
}

void GamepadsSection()
{
    OriginalUI::SeparatorText("Controllers");

    auto nCount = 0;

    const auto pIds = SDLPtr<SDL_JoystickID[]>(SDL_GetGamepads(&nCount));

    if (nCount == 0)
    {
        ImGui::TextDisabled("None plugged in.");
    }

    for (int i = 0; i < nCount; i++)
    {
        auto* const pPad = SDL_GetGamepadFromID(pIds[i]);

        if (pPad == nullptr)
        {
            continue;
        }

        const auto* const pszType = [&]
        {
            const auto* const pszName = SDL_GetGamepadStringForType(SDL_GetGamepadType(pPad));

            if (pszName == nullptr)
            {
                return "unknown";
            }

            return pszName;
        }();

        ImGui::Text("%s (%s)", SDL_GetGamepadName(pPad), pszType);

        if (pIds[i] == Gamepad::LastUsed())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 0.5f, 0.0f, 1.0f), "last used");
        }

        ImGui::Indent();

        ImGui::Text("Left stick %+6d %+6d   Right stick %+6d %+6d", SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_LEFTX), SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_LEFTY), SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_RIGHTX), SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_RIGHTY));
        ImGui::Text("Triggers %5d %5d", SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER), SDL_GetGamepadAxis(pPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));

        ImGui::Text("Held:");

        for (int nButton = 0; nButton < SDL_GAMEPAD_BUTTON_COUNT; nButton++)
        {
            if (SDL_GetGamepadButton(pPad, static_cast<SDL_GamepadButton>(nButton)))
            {
                ImGui::SameLine();
                ImGui::TextUnformatted(SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(nButton)));
            }
        }

        ImGui::Unindent();
    }
}

void InputTab()
{
    auto bEscapePauses = Config::EscapePauses();

    if (OriginalUI::Checkbox("Escape pauses the match", &bEscapePauses))
    {
        Config::SetEscapePauses(bEscapePauses);
    }

    OriginalUI::SetItemTooltip("Escape stops play under its quit prompt until you answer it. Off keeps the match running, as the original did. A network match never pauses.");

    if (InputLog::Count() > 0)
    {
        ImGui::Text("Last used: %s", InputLog::Recent(0).pszSource);
    }

    GamepadsSection();

    OriginalUI::SeparatorText("Keyboard and mouse");

    auto flX = 0.0f;
    auto flY = 0.0f;

    const auto buttons = SDL_GetMouseState(&flX, &flY);

    ImGui::Text("Mouse %.0f, %.0f, buttons 0x%x", flX, flY, static_cast<unsigned int>(buttons));
    ImGui::Text("Keys held:");

    for (unsigned int nKey = 0; nKey < 256; nKey++)
    {
        if (InputLog::IsHeld(nKey))
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(KeyMap::Name(nKey));
        }
    }

    OriginalUI::SeparatorText("Bindings");

    if (ImGui::BeginTable("bindings", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("Action");
        ImGui::TableSetupColumn("Key");
        ImGui::TableSetupColumn("Controller");
        ImGui::TableHeadersRow();

        const auto* const pnKeys = KeyBindings();

        for (int i = 0; i < kKeyBindingCount; i++)
        {
            const auto bHeld = InputLog::IsHeld(pnKeys[i]);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            if (bHeld)
            {
                ImGui::TextColored(ImVec4(0.0f, 0.5f, 0.0f, 1.0f), "%s", g_apszKeyBindingNames[i]);
            }
            else
            {
                ImGui::TextUnformatted(g_apszKeyBindingNames[i]);
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(KeyMap::Name(pnKeys[i]));
            ImGui::TableNextColumn();

            // The first four are the directions, which the left stick steers.
            if (i < 4)
            {
                ImGui::TextUnformatted("LeftStick");
            }

            for (int nControl = 0; nControl < Gamepad::BindingCount(); nControl++)
            {
                const auto binding = Gamepad::GetBinding(nControl);

                if (SDL_strcasecmp(binding.pszAction, g_apszKeyBindingNames[i]) == 0)
                {
                    ImGui::TextUnformatted(binding.pszControl);
                    ImGui::SameLine();
                }
            }
        }

        ImGui::EndTable();
    }

    OriginalUI::SeparatorText("Recent");

    if (ImGui::BeginTable("recent", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 220.0f)))
    {
        ImGui::TableSetupColumn("Ago");
        ImGui::TableSetupColumn("From");
        ImGui::TableSetupColumn("Key");
        ImGui::TableSetupColumn("Action");
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        const auto now = Clock::Now();

        for (int i = 0; i < InputLog::Count(); i++)
        {
            const auto& entry = InputLog::Recent(i);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%.1fs", std::chrono::duration<double>(now - entry.stamp).count());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(entry.pszSource);
            ImGui::TableNextColumn();

            if (entry.bDown)
            {
                ImGui::Text("%s down", KeyMap::Name(entry.nKey));
            }
            else
            {
                ImGui::TextDisabled("%s up", KeyMap::Name(entry.nKey));
            }

            ImGui::TableNextColumn();

            if (entry.pszAction != nullptr)
            {
                ImGui::TextUnformatted(entry.pszAction);
            }
        }

        ImGui::EndTable();
    }
}

struct SlotRun
{
    int         nFirst;
    int         nLast;
    const char* pszName;
};

// The slots objectclasses.h names; a run of more than one numbers its members from zero.
constexpr SlotRun SLOT_RUNS[] = {
    { kSlotDromer0, kSlotDromer3, "Dromer" },
    { kGoalieSlotA, kGoalieSlotA, "Goalie A" },
    { kGoalieSlotB, kGoalieSlotB, "Goalie B" },
    { kSlotRok, kSlotRok, "Rok" },
    { kSlotNgoal, kSlotNgoal, "North goal" },
    { kSlotSgoal, kSlotSgoal, "South goal" },
    { kCameraMainSlot, kCameraMainSlot, "Main camera" },
    { kSlotDrome, kSlotDrome, "Drome" },
    { kSlotRamp, kSlotRamp, "Ramp" },
    { kSlotGod, kSlotGod, "God" },
    { kSlotSoundMan, kSlotSoundMan, "Sound man" },
    { kCameraHeadSlot, kCameraHeadSlot, "Head cam" },
    { 26, 26, "Start gun" }, // TODO: Name in objectclasses.h; the STARTGUN plan times itself out to 26.
    { kSlotMultiplier, kSlotMultiplierLast, "Multiplier" },
    { kSlotTurnstile, kSlotTurnstileLast, "Turnstile" },
    { kSlotLimboPole, kSlotLimboPoleLast, "Limbo pole" },
    { kSlotLaserHurdle, kSlotLaserHurdleLast, "Laser hurdle" },
    { kSlotArmory, kSlotArmoryLast, "Armory" },
    { kSlotMine, kSlotMine, "Mine" },
    { kSlotSwarf, kSlotSwarf, "Swarf" },
    { kSlotFastLane, kSlotFastLane, "Fast lane" },
    { kSlotSlowLane, kSlotSlowLane, "Slow lane" },
    { kSlotExplosion, kSlotExplosion, "Explosion" },
    { kSlotHurdle, kSlotHurdleLast, "Hurdle" },
    { kSlotGreenRandomizer, kSlotRedRandomizer - 1, "Green randomizer" },
    { kSlotRedRandomizer, kSlotRedRandomizerLast, "Red randomizer" },
    { kSlotKeyboard, kSlotKeyboard, "Keyboard" },
    { kSlotGamepad0, kSlotGamepad0, "Gamepad 0" },
    { kSlotCoach0, kSlotCoach0, "Coach 0" },
    { kSlotCoach1, kSlotCoach1, "Coach 1" },
    { kSlotBodyPart, kSlotBodyPartLast, "Body part" },
};

void SlotText(int nSlot)
{
    ImGui::TextUnformatted(DebugMenu::SlotName(nSlot).c_str());
    OriginalUI::SetItemTooltip("Slot %d", nSlot);
}

const char* StateText(EngineObject& object)
{
    if (object.IsHidden() != 0)
    {
        return "Hidden";
    }

    if (object.IsVisible() == 0)
    {
        return "Off screen";
    }

    return "Shown";
}

// Each engine object as the renderer saw it on the last frame.
void ObjectsTab()
{
    if (g_pGameEngine == nullptr || g_pGameEngine->m_ppObjects == nullptr)
    {
        ImGui::TextDisabled("No match loaded.");
        return;
    }

    OriginalUI::Checkbox("Show hidden objects", &s_bShowHidden);

    constexpr auto kFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;

    if (!ImGui::BeginTable("objects", 7, kFlags, ImVec2(0.0f, 360.0f)))
    {
        return;
    }

    for (const auto* pszHeading : { "Object", "State", "Posed", "Detail", "Pieces shown", "Sort bias", "Position" })
    {
        ImGui::TableSetupColumn(pszHeading);
    }

    ImGui::TableSetupScrollFreeze(1, 1);
    ImGui::TableHeadersRow();

    for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
    {
        auto* const pObject = g_pGameEngine->m_ppObjects[i];

        if (!s_bShowHidden && pObject->IsHidden() != 0)
        {
            continue;
        }

        auto nShown    = 0;
        auto flMinBias = 0.0f;
        auto flMaxBias = 0.0f;

        for (short nBone = 0; nBone < pObject->m_wBoneCount; nBone++)
        {
            const MeshBone& bone = pObject->m_pBones[nBone];

            if (bone.m_pMesh == nullptr)
            {
                continue;
            }

            if (bone.IsHidden() == 0)
            {
                nShown++;
            }

            flMinBias = (std::min)(flMinBias, bone.m_pMesh->flDepthBias);
            flMaxBias = (std::max)(flMaxBias, bone.m_pMesh->flDepthBias);
        }

        const Vec3& vPosition = *pObject->GetPositionPtr();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        SlotText(pObject->GetSlot());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(StateText(*pObject));
        ImGui::TableNextColumn();

        if ((pObject->m_nFlags & kEngineObjectPosed) != 0)
        {
            ImGui::TextUnformatted("Yes");
        }

        ImGui::TableNextColumn();
        ImGui::Text("LOD %d", pObject->m_wLodLevel);
        ImGui::TableNextColumn();
        ImGui::Text("%d of %d", nShown, pObject->m_wBoneCount);
        ImGui::TableNextColumn();

        if (flMinBias == flMaxBias)
        {
            ImGui::Text("%.1f", flMinBias);
        }
        else
        {
            ImGui::Text("%.1f to %.1f", flMinBias, flMaxBias);
        }

        ImGui::TableNextColumn();
        ImGui::Text("%.0f, %.0f, %.0f", vPosition.x, vPosition.y, vPosition.z);
    }

    ImGui::EndTable();
}

// The original's typed cheats, as buttons. The tooltip decodes the code a player types for each.
struct CheatButton
{
    void (*pfnActivate)();
    const char* pszLabel;
    bool        bInMatch;
};

constexpr CheatButton CHEATS[] = {
    { Cheat::EnableHiddenTeams, "Unlock all teams", false },
    { Cheat::EnableAllAwards, "Unlock all trophies", false },
    { Cheat::MaxViewedPlayerRatings, "Max the viewed player's ratings", true },
    { Cheat::BroadcastFreakout, "Freakout", true },
    { Cheat::GrowPlayers, "Grow players", true },
    { Cheat::ShrinkPlayers, "Shrink players", true },
    { Cheat::StretchPlayers, "Stretch players", true },
    { Cheat::BulkUpPlayers, "Bulk up players", true },
    { Cheat::SquashPlayers, "Squash players", true },
    { Cheat::MirrorPlayers, "Mirror players", true },
    { Cheat::FlipPlayers, "Turn players upside down", true },
    { Cheat::FlipPlayerLegs, "Flip players' legs", true },
    { Cheat::RestoreAllPlayers, "Restore players' shapes", true },
    { Cheat::PlayActivateSound, "Play the cheat sound", false },
    { Cheat::GeorgeActivate, "George", false },
};

// The code the table stores for a cheat, each character exclusive-ored with its key.
std::string CheatCode(void (*pfnActivate)())
{
    for (const auto& entry : g_aCheatCodes)
    {
        if (entry.pfnActivate != pfnActivate)
        {
            continue;
        }

        const auto nLast = static_cast<unsigned char>(entry.pszCode[0]);
        const auto nKey  = static_cast<unsigned char>(entry.pszCode[1]);

        auto code = std::string();

        for (unsigned int i = 0; i <= nLast; i++)
        {
            code += static_cast<char>(static_cast<unsigned char>(entry.pszCode[2 + i]) ^ nKey);
        }

        return code;
    }

    return std::string();
}

void CheatsTab()
{
    const auto bInMatch = g_pGameEngine != nullptr && g_gameApp.m_nEngineState == kEstateMatchLive;

    OriginalUI::SeparatorText("Unlocks");

    for (const auto& cheat : CHEATS)
    {
        if (cheat.bInMatch)
        {
            continue;
        }

        ImGui::BeginDisabled(g_pGameEngine == nullptr);

        if (OriginalUI::Button(cheat.pszLabel))
        {
            cheat.pfnActivate();
        }

        ImGui::EndDisabled();
        OriginalUI::SetItemTooltip("Typed in a match as %s", CheatCode(cheat.pfnActivate).c_str());
    }

    OriginalUI::SeparatorText("In a match");

    if (!bInMatch)
    {
        ImGui::TextDisabled("Start a match to use these.");
    }

    for (const auto& cheat : CHEATS)
    {
        if (!cheat.bInMatch)
        {
            continue;
        }

        ImGui::BeginDisabled(!bInMatch);

        if (OriginalUI::Button(cheat.pszLabel))
        {
            cheat.pfnActivate();
        }

        ImGui::EndDisabled();
        OriginalUI::SetItemTooltip("Typed in a match as %s", CheatCode(cheat.pfnActivate).c_str());
    }
}

void SpectateTab()
{
    ImGui::BeginDisabled(g_bSessionActive != 0);

    auto bAutopilot = Spectator::AutopilotSetting();

    if (OriginalUI::Checkbox("Autopilot", &bAutopilot))
    {
        Spectator::SetAutopilot(bAutopilot);
    }

    OriginalUI::SetItemTooltip("Every dromer plays on the AI plans, whoever control passes to. Start with -b to have it from the off. Tab's side view turns it on while it is up.");
    ImGui::EndDisabled();

    auto bFreeCamera = Spectator::FreeCamera();

    if (OriginalUI::Checkbox("Free camera (F3)", &bFreeCamera))
    {
        Spectator::SetFreeCamera(bFreeCamera);
    }

    ImGui::TextDisabled("WASD moves, Q and E sink and rise, Shift hurries.");
    ImGui::TextDisabled("Hold the right mouse button to look. A pad's sticks and triggers fly it too.");

    if (Spectator::HoldsInput())
    {
        ImGui::TextDisabled("Your keys reach only the match keys while either is on.");
    }
}

void PlansTab()
{
    ImGui::BeginDisabled(g_bSessionActive != 0);

    auto bFixed = PlanFixes::On();

    if (OriginalUI::Checkbox("Fixed plans", &bFixed))
    {
        PlanFixes::Set(bFixed);
    }

    OriginalUI::SetItemTooltip("The fixes for the bugs docs/plan_bugs.md lists in the shipped AI and object plans, and the engine faults they hit. Off runs the plans as shipped.");
    ImGui::EndDisabled();

    ImGui::TextDisabled("Both sets are compiled at load; switching restarts each object's current behaviour.");
    ImGui::TextDisabled("The team file fixes take effect when the next match loads its teams.");

    if (g_bSessionActive != 0)
    {
        ImGui::TextDisabled("Locked during a network session, as every player must run the same plans.");
    }
}

// The window's tabs in order, which the Debug menu lists too.
constexpr std::pair<const char*, void (*)()> s_aTabs[] = {
    { "Graphics", GraphicsTab },
    { "Lighting", LightingTab },
    { "Timing", TimingTab },
    { "Input", InputTab },
    { "Objects", ObjectsTab },
    { "Cheats", CheatsTab },
    { "Spectate", SpectateTab },
    { "Plans", PlansTab },
};

// The tab the Debug menu last picked, selected when the window next draws.
const char* s_pszPickedTab = nullptr;

void DebugMenuOnBar()
{
    if (!OriginalUI::BeginMenu("Debug", true))
    {
        return;
    }

    if (OriginalUI::MenuItem("Debug window", "F2", s_bOpen, true))
    {
        s_bOpen = !s_bOpen;
    }

    OriginalUI::Separator();

    for (const auto& [pszName, pfnTab] : s_aTabs)
    {
        if (OriginalUI::MenuItem(pszName, nullptr, false, true))
        {
            s_bOpen        = true;
            s_pszPickedTab = pszName;
        }
    }

    OriginalUI::Separator();

    constexpr std::pair<const char*, Scenes::Scene> SCENES[] = {
        { "Model viewer", Scenes::Scene::ModelViewer },
        { "Sound player", Scenes::Scene::SoundPlayer },
        { "Image browser", Scenes::Scene::ImageBrowser },
    };

    for (const auto& [pszName, scene] : SCENES)
    {
        const auto bOpen = Scenes::Current() == scene;

        if (!OriginalUI::MenuItem(pszName, nullptr, bOpen, true))
        {
            continue;
        }

        if (bOpen)
        {
            Scenes::Open(Scenes::Scene::None);
        }
        else
        {
            Scenes::Open(scene);
        }
    }

    ImGui::EndMenu();
}

void Build()
{
    ImGui::SetNextWindowPos(ImVec2(16, 16 + ImGui::GetFrameHeight()), ImGuiCond_FirstUseEver);

    const auto theme = OriginalUI::Theme();

    // Its slider takes a sunken edge from ImGui; the other controls draw their own.
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    ImGui::Begin("HyperBlade 20XX", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);

    if (!OriginalUI::DrawWindowFrame("HyperBlade 20XX"))
    {
        s_bOpen = false;
    }

    ImGui::Text("%.0f FPS, %d ticks a second", ImGui::GetIO().Framerate, FramePacing::TicksPerSecond());

    if (OriginalUI::BeginTabBar("tabs"))
    {
        for (const auto& [pszName, pfnTab] : s_aTabs)
        {
            if (OriginalUI::BeginTabItem(pszName, pszName == s_pszPickedTab))
            {
                pfnTab();
                OriginalUI::EndTabItem();
            }
        }

        s_pszPickedTab = nullptr;

        OriginalUI::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace

void DebugMenu::Toggle()
{
    s_bOpen = !s_bOpen;
}

void DebugMenu::OnEvent(Event event)
{
    if (s_bReady)
    {
        ImGui_ImplSDL3_ProcessEvent(event.As<SDL_Event>());
    }
}

bool DebugMenu::WantsMouse()
{
    return s_bReady && (Scenes::Current() != Scenes::Scene::None || (AnythingShown() && ImGui::GetIO().WantCaptureMouse));
}

bool DebugMenu::WantsKeyboard()
{
    return s_bReady && (Scenes::Current() != Scenes::Scene::None || (AnythingShown() && ImGui::GetIO().WantCaptureKeyboard));
}

void DebugMenu::Draw(Renderer renderer)
{
    auto* const pRenderer = renderer.As<SDL_Renderer>();

    Ready(pRenderer);

    if (!AnythingShown())
    {
        return;
    }

    const auto bMenuBar = MenuBarShown();

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (bMenuBar)
    {
        const auto theme = OriginalUI::Theme();

        if (ImGui::BeginMainMenuBar())
        {
            OriginalUI::DrawMenus();
            DebugMenuOnBar();
            ImGui::EndMainMenuBar();
        }

        s_flMenuBarHeight = ImGui::GetFrameHeight();
    }

    if (s_bOpen)
    {
        Build();
    }

    OriginalUI::DrawDialogs();
    Scenes::DrawWindows();

    // The bar stays while the pointer is over it or one of its menus is open.
    s_bMenuBarKept = bMenuBar && (PointerAtTop() || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId));

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), pRenderer);
}

std::string DebugMenu::SlotName(int nSlot)
{
    for (const auto& run : SLOT_RUNS)
    {
        if (nSlot < run.nFirst || nSlot > run.nLast)
        {
            continue;
        }

        if (run.nFirst == run.nLast)
        {
            return run.pszName;
        }

        return std::string(run.pszName) + " " + std::to_string(nSlot - run.nFirst);
    }

    return "Slot " + std::to_string(nSlot);
}

void DebugMenu::RendererControls()
{
    if (!OriginalUI::BeginTabBar("renderer"))
    {
        return;
    }

    for (const auto& [pszName, pfnTab] : { std::pair{ "Graphics", GraphicsTab }, std::pair{ "Lighting", LightingTab } })
    {
        if (OriginalUI::BeginTabItem(pszName, false))
        {
            pfnTab();
            OriginalUI::EndTabItem();
        }
    }

    OriginalUI::EndTabBar();
}
