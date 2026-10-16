#include "common/compat.h"
#include "ui/debugmenu.h"
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
#include <SDL3/SDL.h>
#include <algorithm>
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

struct SlotRun
{
    int         nFirst;
    int         nLast;
    const char* pszName;
};

// The slots objectclasses.h names; a run of more than one numbers its members from zero.
constexpr SlotRun SLOT_RUNS[] = {
    { SLOT_DROMER0, SLOT_DROMER3, "Dromer" },
    { GOALIE_SLOT_A, GOALIE_SLOT_A, "Goalie A" },
    { GOALIE_SLOT_B, GOALIE_SLOT_B, "Goalie B" },
    { SLOT_ROK, SLOT_ROK, "Rok" },
    { SLOT_NGOAL, SLOT_NGOAL, "North goal" },
    { SLOT_SGOAL, SLOT_SGOAL, "South goal" },
    { CAMERA_MAIN_SLOT, CAMERA_MAIN_SLOT, "Main camera" },
    { SLOT_DROME, SLOT_DROME, "Drome" },
    { SLOT_RAMP, SLOT_RAMP, "Ramp" },
    { SLOT_GOD, SLOT_GOD, "God" },
    { SLOT_SOUND_MAN, SLOT_SOUND_MAN, "Sound man" },
    { CAMERA_HEAD_SLOT, CAMERA_HEAD_SLOT, "Head cam" },
    { 26, 26, "Start gun" }, // TODO: Name in objectclasses.h; the STARTGUN plan times itself out to 26.
    { SLOT_MULTIPLIER, SLOT_MULTIPLIER_LAST, "Multiplier" },
    { SLOT_TURNSTILE, SLOT_TURNSTILE_LAST, "Turnstile" },
    { SLOT_LIMBO_POLE, SLOT_LIMBO_POLE_LAST, "Limbo pole" },
    { SLOT_LASER_HURDLE, SLOT_LASER_HURDLE_LAST, "Laser hurdle" },
    { SLOT_ARMORY, SLOT_ARMORY_LAST, "Armory" },
    { SLOT_MINE, SLOT_MINE, "Mine" },
    { SLOT_SWARF, SLOT_SWARF, "Swarf" },
    { SLOT_FAST_LANE, SLOT_FAST_LANE, "Fast lane" },
    { SLOT_SLOW_LANE, SLOT_SLOW_LANE, "Slow lane" },
    { SLOT_EXPLOSION, SLOT_EXPLOSION, "Explosion" },
    { SLOT_HURDLE, SLOT_HURDLE_LAST, "Hurdle" },
    { SLOT_GREEN_RANDOMIZER, SLOT_RED_RANDOMIZER - 1, "Green randomizer" },
    { SLOT_RED_RANDOMIZER, SLOT_RED_RANDOMIZER_LAST, "Red randomizer" },
    { SLOT_KEYBOARD, SLOT_KEYBOARD, "Keyboard" },
    { SLOT_GAMEPAD0, SLOT_GAMEPAD0, "Gamepad 0" },
    { SLOT_COACH0, SLOT_COACH0, "Coach 0" },
    { SLOT_COACH1, SLOT_COACH1, "Coach 1" },
    { SLOT_BODY_PART, SLOT_BODY_PART_LAST, "Body part" },
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

    constexpr auto FLAGS = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;

    if (!ImGui::BeginTable("objects", 7, FLAGS, ImVec2(0.0f, 360.0f)))
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
        EngineObject* const pObject = g_pGameEngine->m_ppObjects[i];

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

        if ((pObject->m_nFlags & ENGINEOBJECT_POSED) != 0)
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
        ObjectsTab();

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
    SDL_Renderer* const pRenderer = renderer.As<SDL_Renderer>();

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
