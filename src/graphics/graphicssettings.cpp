#include "common/compat.h"
#include "graphics/graphicssettings.h"
#include "graphics/presenter.h"
#include "engine/gameoptions.h"
#include "globals.h"
#include <array>
#include <cmath>

namespace
{

constexpr auto kSection     = "Graphics";
constexpr auto kRendererKey = "Renderer";

struct SwitchInfo
{
    const char* pszKey;
    bool        bDefault;
};

constexpr auto kSwitches = std::array<SwitchInfo, static_cast<size_t>(GraphicsSettings::Switch::COUNT)>{ {
    { "Textures", true },
    { "LinearFiltering", false },
    { "Uncapped", true },
    { "FullDetail", true },
    { "SmoothShading", false },
    { "PhongShading", false },
    { "Light303", true },
    { "Light54", true },
    { "Light180", true },
    { "SeveralLights", false },
    { "RokLight", false },
    { "ShadowMaps", true },
    { "PaintersOrder", true },
} };

auto s_bLoaded          = false;
auto s_renderer         = GraphicsSettings::Renderer::HARDWARE;
auto s_abOn             = std::array<bool, kSwitches.size()>{};
auto s_flRokLightRadius = 200.0f;

// Saved in hundredths of a world unit, as the options file holds whole numbers.
constexpr auto kShadowSoftnessKey     = "ShadowSoftness";
constexpr auto kShadowSoftnessDefault = 12;

auto s_flShadowSoftness = kShadowSoftnessDefault / 100.0f;

void Load()
{
    if (s_bLoaded || g_pGameOptions == nullptr)
    {
        return;
    }

    s_bLoaded = true;

    if (g_pGameOptions->ReadInt(kSection, kRendererKey, 1) == 0)
    {
        s_renderer = GraphicsSettings::Renderer::SOFTWARE;
    }

    for (size_t i = 0; i < kSwitches.size(); i++)
    {
        s_abOn[i] = g_pGameOptions->ReadInt(kSection, kSwitches[i].pszKey, static_cast<int>(kSwitches[i].bDefault)) != 0;
    }

    s_flShadowSoftness = static_cast<int>(g_pGameOptions->ReadInt(kSection, kShadowSoftnessKey, kShadowSoftnessDefault)) / 100.0f;
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

    g_pGameOptions->WriteInt(kSection, kRendererKey, static_cast<int>(s_renderer));
}

bool GraphicsSettings::On(Switch which)
{
    Load();

    // Before the options load, every switch reads as its default.
    if (!s_bLoaded)
    {
        return kSwitches[static_cast<size_t>(which)].bDefault;
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

    g_pGameOptions->WriteInt(kSection, kSwitches[static_cast<size_t>(which)].pszKey, static_cast<int>(bOn));
}

float GraphicsSettings::RokLightRadius()
{
    return s_flRokLightRadius;
}

void GraphicsSettings::SetRokLightRadius(float flRadius)
{
    s_flRokLightRadius = flRadius;
}

float GraphicsSettings::ShadowSoftness()
{
    Load();

    return s_flShadowSoftness;
}

void GraphicsSettings::SetShadowSoftness(float flRadius)
{
    Load();

    s_flShadowSoftness = flRadius;
}

void GraphicsSettings::SaveShadowSoftness()
{
    if (g_pGameOptions != nullptr)
    {
        g_pGameOptions->WriteInt(kSection, kShadowSoftnessKey, static_cast<int>(std::lround(s_flShadowSoftness * 100.0f)));
    }
}
