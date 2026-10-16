#include "common/compat.h"
#include "ui/debugmenu.h"
#include "app/window.h"
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

void Checkbox(const char* pszLabel, bool bValue, void (*pToggle)())
{
    if (ImGui::Checkbox(pszLabel, &bValue))
    {
        pToggle();
    }
}

void Build()
{
    ImGui::SetNextWindowPos(ImVec2(16, 16), ImGuiCond_FirstUseEver);

    ImGui::Begin("HyperBlade 20XX", &s_bOpen, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate);

    ImGui::SeparatorText("Renderer");

    ImGui::BeginDisabled(!Presenter::HasGPU());
    Checkbox("Hardware", GraphicsSettings::Active() == GraphicsSettings::Renderer::HARDWARE, GraphicsSettings::ToggleRenderer);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(GraphicsSettings::Active() != GraphicsSettings::Renderer::HARDWARE);
    Checkbox("Textures", GraphicsSettings::Textures(), GraphicsSettings::ToggleTextures);
    Checkbox("Linear filtering", GraphicsSettings::LinearFiltering(), GraphicsSettings::ToggleLinearFiltering);
    Checkbox("Full detail models", GraphicsSettings::FullDetail(), GraphicsSettings::ToggleFullDetail);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Timing");

    Checkbox("Uncapped frame rate", GraphicsSettings::Uncapped(), GraphicsSettings::ToggleUncapped);

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
