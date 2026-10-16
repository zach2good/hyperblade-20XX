#include "common/compat.h"
#include "ui/debugmenu.h"
#include "app/window.h"
#include "engine/framepacing.h"
#include "graphics/graphicssettings.h"
#include "graphics/presenter.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

namespace
{

bool s_bReady = false;
bool s_bOpen  = false;

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

void Checkbox(const char* pszLabel, GraphicsSettings::Switch which)
{
    auto bOn = GraphicsSettings::On(which);

    if (ImGui::Checkbox(pszLabel, &bOn))
    {
        GraphicsSettings::Toggle(which);
    }
}

void Build()
{
    using GraphicsSettings::Switch;

    ImGui::SetNextWindowPos(ImVec2(16, 16), ImGuiCond_FirstUseEver);

    ImGui::Begin("HyperBlade 20XX", &s_bOpen, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate);

    ImGui::SeparatorText("Renderer");

    const auto bHardware = GraphicsSettings::Active() == GraphicsSettings::Renderer::HARDWARE;

    ImGui::BeginDisabled(!Presenter::HasGPU());

    auto bHardwareBox = bHardware;

    if (ImGui::Checkbox("Hardware", &bHardwareBox))
    {
        GraphicsSettings::ToggleRenderer();
    }

    ImGui::EndDisabled();

    ImGui::BeginDisabled(!bHardware);
    Checkbox("Textures", Switch::TEXTURES);
    Checkbox("Linear filtering", Switch::LINEAR_FILTERING);
    Checkbox("Full detail models", Switch::FULL_DETAIL);
    Checkbox("Smooth shading", Switch::SMOOTH_SHADING);

    ImGui::SeparatorText("Lights");

    Checkbox("Light at 303", Switch::LIGHT_303);
    Checkbox("Light at 54", Switch::LIGHT_54);
    Checkbox("Light at 180, from below", Switch::LIGHT_180);
    Checkbox("Several lights", Switch::SEVERAL_LIGHTS);
    Checkbox("Rok light", Switch::ROK_LIGHT);

    auto flRadius = GraphicsSettings::RokLightRadius();

    if (ImGui::SliderFloat("Rok light reach", &flRadius, 10.0f, 2000.0f, "%.0f", ImGuiSliderFlags_Logarithmic))
    {
        GraphicsSettings::SetRokLightRadius(flRadius);
    }

    ImGui::SeparatorText("Shadows");

    Checkbox("Shadow maps", Switch::SHADOW_MAPS);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Timing");

    Checkbox("Uncapped frame rate (experimental)", Switch::UNCAPPED);

    ImGui::Text("%d ticks/s, clock drift %+.0f ms/s", FramePacing::TicksPerSecond(), FramePacing::DriftMs());

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
