#include "common/compat.h"
#include "common/clock.h"
#include "ui/debugmenu.h"
#include "app/gameapp.h"
#include "app/window.h"
#include "engine/engineobject.h"
#include "engine/framepacing.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
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

    ImGui_ImplSDL3_InitForSDLRenderer(Window::Native().As<SDL_Window>(), pRenderer);
    ImGui_ImplSDLRenderer3_Init(pRenderer);

    s_bReady = true;
}

void Checkbox(const char* pszLabel, Switch which, const char* pszTooltip)
{
    auto bOn = GraphicsSettings::On(which);

    if (ImGui::Checkbox(pszLabel, &bOn))
    {
        GraphicsSettings::Toggle(which);
    }

    ImGui::SetItemTooltip("%s", pszTooltip);
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
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "The software renderer draws the original look,");
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "so these only take effect on the hardware renderer.");

        ImGui::BeginDisabled(!Presenter::HasGPU());

        if (ImGui::Button("Switch to hardware"))
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
    if (!ImGui::BeginTabItem("Graphics"))
    {
        return;
    }

    ImGui::SeparatorText("Renderer");

    ImGui::BeginDisabled(!Presenter::HasGPU());

    if (ImGui::RadioButton("Original software renderer", !Hardware()) && Hardware())
    {
        GraphicsSettings::ToggleRenderer();
    }

    ImGui::SameLine();

    if (ImGui::RadioButton("New hardware renderer", Hardware()) && !Hardware())
    {
        GraphicsSettings::ToggleRenderer();
    }

    ImGui::EndDisabled();

    if (!Presenter::HasGPU())
    {
        ImGui::TextDisabled("No GPU device, so software only.");
    }

    BeginHardwareOnly();

    ImGui::SeparatorText("Models");
    Checkbox("Textured models", Switch::TEXTURES, "The textured model set, as the original's Direct3D mode drew it. Off draws the shaded models in palette colours.");
    Checkbox("Full detail at any distance", Switch::FULL_DETAIL, "Always the most detailed textured model. Off steps down with distance, as the engine picks.");

    auto bDrome = g_nDromeTextured != 0;

    ImGui::BeginDisabled(g_bShadedDromeModels != 0);

    if (ImGui::Checkbox("Textured drome (F8)", &bDrome))
    {
        Renderer::ToggleDromeTextured();
    }

    ImGui::SetItemTooltip("The original's F8: the drome with its textures, or shaded. The -m switch keeps it shaded.");
    ImGui::EndDisabled();

    ImGui::SeparatorText("Surfaces");
    Checkbox("Smooth texture filtering", Switch::LINEAR_FILTERING, "Linear filtering, as the original's F7 toggled. Off is nearest.");

    ImGui::SeparatorText("Shading");

    const auto bSmooth = GraphicsSettings::On(Switch::SMOOTH_SHADING);
    const auto bPhong  = GraphicsSettings::On(Switch::PHONG_SHADING);

    if (ImGui::RadioButton("Flat (original)", !bSmooth))
    {
        if (bSmooth)
        {
            GraphicsSettings::Toggle(Switch::SMOOTH_SHADING);
        }
    }

    ImGui::SetItemTooltip("Each face lit as one, as the original did.");
    ImGui::SameLine();

    if (ImGui::RadioButton("Gouraud shading", bSmooth && !bPhong))
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

    ImGui::SetItemTooltip("Light worked out at each corner and blended across the face, smooth across edges that don't crease.");
    ImGui::SameLine();

    if (ImGui::RadioButton("Phong shading", bSmooth && bPhong))
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

    ImGui::SetItemTooltip("Light worked out at each pixel from the blended normal, for the smoothest highlights.");

    ImGui::SeparatorText("Draw order");
    Checkbox("Paint in original order", Switch::PAINTERS_ORDER, "Each mesh covers the ones sorted behind it, as the original painted them. Off depth tests every mesh against the others.");

    ImGui::EndDisabled();
    ImGui::EndTabItem();
}

void LightingTab()
{
    if (!ImGui::BeginTabItem("Lighting"))
    {
        return;
    }

    BeginHardwareOnly();

    ImGui::SeparatorText("Scene lights");
    Checkbox("Light from 303\xc2\xb0", Switch::LIGHT_303, "One of the two lights above the arena.");
    Checkbox("Light from 54\xc2\xb0", Switch::LIGHT_54, "The other light above the arena.");
    Checkbox("Light from below", Switch::LIGHT_180, "Lights the drome's first piece from underneath.");
    Checkbox("Fill from every light", Switch::SEVERAL_LIGHTS, "The original lights each face by one light. On, the other lights fill in where they face it.");

    ImGui::SeparatorText("Rok");
    Checkbox("Rok emits light", Switch::ROK_LIGHT, "A point light carried by the rok, added over the scene lights.");

    ImGui::BeginDisabled(!GraphicsSettings::On(Switch::ROK_LIGHT));

    auto flRadius = GraphicsSettings::RokLightRadius();

    if (ImGui::SliderFloat("Reach", &flRadius, 10.0f, 2000.0f, "%.0f", ImGuiSliderFlags_Logarithmic))
    {
        GraphicsSettings::SetRokLightRadius(flRadius);
    }

    ImGui::SetItemTooltip("How far the glow reaches, in world units. Not saved.");
    ImGui::EndDisabled();

    ImGui::SeparatorText("Shadows");
    Checkbox("Realtime shadows", Switch::SHADOW_MAPS, "Objects cast shadow maps from both lights above the arena, in place of the original's shadow shapes.");

    ImGui::EndDisabled();
    ImGui::EndTabItem();
}

void TimingTab()
{
    if (!ImGui::BeginTabItem("Timing"))
    {
        return;
    }

    ImGui::SeparatorText("Frame rate");
    Checkbox("Uncapped (experimental)", Switch::UNCAPPED, "A match draws as often as it can, blending between the 30 Hz simulation ticks. Off draws once a tick.");

    ImGui::SeparatorText("Simulation");
    ImGui::Text("%d ticks a second", FramePacing::TicksPerSecond());
    ImGui::SetItemTooltip("Should read 30 in a match.");
    ImGui::Text("Clock drift %+.0f ms a second", FramePacing::DriftMs());
    ImGui::SetItemTooltip("How far the ticks ran ahead of the wall clock. Should read near 0.");

    ImGui::EndTabItem();
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
    ImGui::SeparatorText("Controllers");

    auto nCount = 0;

    auto* const pIds = SDL_GetGamepads(&nCount);

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
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "last used");
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

    SDL_free(pIds);
}

void InputTab()
{
    if (!ImGui::BeginTabItem("Input"))
    {
        return;
    }

    if (InputLog::Count() > 0)
    {
        ImGui::Text("Last used: %s", InputLog::Recent(0).pszSource);
    }

    GamepadsSection();

    ImGui::SeparatorText("Keyboard and mouse");

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

    ImGui::SeparatorText("Bindings");

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
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", g_apszKeyBindingNames[i]);
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

    ImGui::SeparatorText("Recent");

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

    ImGui::EndTabItem();
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
    for (const auto& run : SLOT_RUNS)
    {
        if (nSlot < run.nFirst || nSlot > run.nLast)
        {
            continue;
        }

        if (run.nFirst == run.nLast)
        {
            ImGui::Text("%s", run.pszName);
        }
        else
        {
            ImGui::Text("%s %d", run.pszName, nSlot - run.nFirst);
        }

        ImGui::SetItemTooltip("Slot %d", nSlot);

        return;
    }

    ImGui::Text("Slot %d", nSlot);
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
    if (!ImGui::BeginTabItem("Objects"))
    {
        return;
    }

    if (g_pGameEngine == nullptr || g_pGameEngine->m_ppObjects == nullptr)
    {
        ImGui::TextDisabled("No match loaded.");
        ImGui::EndTabItem();

        return;
    }

    ImGui::Checkbox("Show hidden objects", &s_bShowHidden);

    constexpr auto kFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;

    if (!ImGui::BeginTable("objects", 7, kFlags, ImVec2(0.0f, 360.0f)))
    {
        ImGui::EndTabItem();

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
    ImGui::EndTabItem();
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
    if (!ImGui::BeginTabItem("Cheats"))
    {
        return;
    }

    const auto bInMatch = g_pGameEngine != nullptr && g_gameApp.m_nEngineState == kEstateMatchLive;

    ImGui::SeparatorText("Unlocks");

    for (const auto& cheat : CHEATS)
    {
        if (cheat.bInMatch)
        {
            continue;
        }

        ImGui::BeginDisabled(g_pGameEngine == nullptr);

        if (ImGui::Button(cheat.pszLabel))
        {
            cheat.pfnActivate();
        }

        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Typed in a match as %s", CheatCode(cheat.pfnActivate).c_str());
    }

    ImGui::SeparatorText("In a match");

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

        if (ImGui::Button(cheat.pszLabel))
        {
            cheat.pfnActivate();
        }

        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Typed in a match as %s", CheatCode(cheat.pfnActivate).c_str());
    }

    ImGui::EndTabItem();
}

void Build()
{
    ImGui::SetNextWindowPos(ImVec2(16, 16), ImGuiCond_FirstUseEver);

    ImGui::Begin("HyperBlade 20XX", &s_bOpen, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%.0f FPS, %d ticks a second", ImGui::GetIO().Framerate, FramePacing::TicksPerSecond());

    if (ImGui::BeginTabBar("tabs"))
    {
        GraphicsTab();
        LightingTab();
        TimingTab();
        InputTab();
        ObjectsTab();
        CheatsTab();

        ImGui::EndTabBar();
    }

    ImGui::End();
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
    return s_bOpen && s_bReady && ImGui::GetIO().WantCaptureMouse;
}

bool DebugMenu::WantsKeyboard()
{
    return s_bOpen && s_bReady && ImGui::GetIO().WantCaptureKeyboard;
}

void DebugMenu::Draw(Renderer renderer)
{
    auto* const pRenderer = renderer.As<SDL_Renderer>();

    Ready(pRenderer);

    if (!s_bOpen)
    {
        return;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    Build();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), pRenderer);
}
