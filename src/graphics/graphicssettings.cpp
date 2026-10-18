#include "common/compat.h"
#include "graphics/graphicssettings.h"
#include "graphics/presenter.h"
#include "engine/gameoptions.h"
#include "globals.h"
#include <array>

namespace
{

constexpr auto SECTION      = "Graphics";
constexpr auto RENDERER_KEY = "Renderer";

struct SwitchInfo
{
    const char* pszKey;
    bool        bDefault;
};

constexpr auto SWITCHES = std::array<SwitchInfo, static_cast<size_t>(GraphicsSettings::Switch::COUNT)>{ {
    { "Textures", true },
    { "LinearFiltering", false },
    { "Uncapped", false },
    { "FullDetail", true },
    { "SmoothShading", false },
    { "PhongShading", false },
    { "Light303", true },
    { "Light54", true },
    { "Light180", true },
    { "SeveralLights", false },
    { "RokLight", false },
    { "ShadowMaps", false },
    { "PaintersOrder", true },
} };

auto s_bLoaded          = false;
auto s_renderer         = GraphicsSettings::Renderer::SOFTWARE;
auto s_abOn             = std::array<bool, SWITCHES.size()>{};
auto s_flRokLightRadius = 200.0f;

void Load()
{
    if (s_bLoaded || g_pGameOptions == nullptr)
    {
        return;
    }

    s_bLoaded = true;

    if (g_pGameOptions->ReadInt(SECTION, RENDERER_KEY, 0) == 1)
    {
        s_renderer = GraphicsSettings::Renderer::HARDWARE;
    }

    for (size_t i = 0; i < SWITCHES.size(); i++)
    {
        s_abOn[i] = g_pGameOptions->ReadInt(SECTION, SWITCHES[i].pszKey, static_cast<int>(SWITCHES[i].bDefault)) != 0;
    }
}

} // namespace

GraphicsSettings::Renderer GraphicsSettings::Active()
{
    Load();

    if (!Presenter::HasGPU())
    {
        return Renderer::SOFTWARE;
    }

    return s_renderer;
}

void GraphicsSettings::ToggleRenderer()
{
    Load();

    if (!Presenter::HasGPU() || g_pGameOptions == nullptr)
    {
        return;
    }

    s_renderer = [&]
    {
        if (s_renderer == Renderer::SOFTWARE)
        {
            return Renderer::HARDWARE;
        }

        return Renderer::SOFTWARE;
    }();

    g_pGameOptions->WriteInt(SECTION, RENDERER_KEY, static_cast<int>(s_renderer));
}

bool GraphicsSettings::On(Switch which)
{
    Load();

    // Before the options load, every switch reads as its default.
    if (!s_bLoaded)
    {
        return SWITCHES[static_cast<size_t>(which)].bDefault;
    }

    return s_abOn[static_cast<size_t>(which)];
}

void GraphicsSettings::Toggle(Switch which)
{
    Load();

    if (g_pGameOptions == nullptr)
    {
        return;
    }

    auto& bOn = s_abOn[static_cast<size_t>(which)];

    bOn = !bOn;

    g_pGameOptions->WriteInt(SECTION, SWITCHES[static_cast<size_t>(which)].pszKey, static_cast<int>(bOn));
}

float GraphicsSettings::RokLightRadius()
{
    return s_flRokLightRadius;
}

void GraphicsSettings::SetRokLightRadius(float flRadius)
{
    s_flRokLightRadius = flRadius;
}
