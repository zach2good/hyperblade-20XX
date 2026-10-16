#include "common/compat.h"
#include "graphics/graphicssettings.h"
#include "graphics/presenter.h"
#include "engine/gameoptions.h"
#include "globals.h"

namespace
{

constexpr auto SECTION       = "Graphics";
constexpr auto RENDERER_KEY  = "Renderer";
constexpr auto TEXTURES_KEY  = "Textures";
constexpr auto FILTERING_KEY = "LinearFiltering";

auto s_bLoaded   = false;
auto s_renderer  = GraphicsSettings::Renderer::SOFTWARE;
auto s_bTextures = true;
auto s_bLinear   = false;

void Load()
{
    if (s_bLoaded || g_pGameOptions == 0)
    {
        return;
    }

    s_bLoaded = true;

    if (g_pGameOptions->ReadInt(SECTION, RENDERER_KEY, 0) == 1)
    {
        s_renderer = GraphicsSettings::Renderer::HARDWARE;
    }

    s_bTextures = g_pGameOptions->ReadInt(SECTION, TEXTURES_KEY, 1) != 0;
    s_bLinear   = g_pGameOptions->ReadInt(SECTION, FILTERING_KEY, 0) != 0;
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

    if (!Presenter::HasGPU() || g_pGameOptions == 0)
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

    g_pGameOptions->WriteInt(SECTION, RENDERER_KEY, (int)s_renderer);
}

bool GraphicsSettings::Textures()
{
    Load();

    return s_bTextures;
}

void GraphicsSettings::ToggleTextures()
{
    Load();

    if (g_pGameOptions == 0)
    {
        return;
    }

    s_bTextures = !s_bTextures;

    g_pGameOptions->WriteInt(SECTION, TEXTURES_KEY, (int)s_bTextures);
}

bool GraphicsSettings::LinearFiltering()
{
    Load();

    return s_bLinear;
}

void GraphicsSettings::ToggleLinearFiltering()
{
    Load();

    if (g_pGameOptions == 0)
    {
        return;
    }

    s_bLinear = !s_bLinear;

    g_pGameOptions->WriteInt(SECTION, FILTERING_KEY, (int)s_bLinear);
}
