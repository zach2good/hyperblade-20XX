#include "common/compat.h"
#include "graphics/graphicssettings.h"
#include "graphics/presenter.h"
#include "engine/gameoptions.h"
#include "globals.h"

namespace
{

constexpr auto SECTION      = "Graphics";
constexpr auto RENDERER_KEY = "Renderer";

auto s_bLoaded  = false;
auto s_renderer = GraphicsSettings::Renderer::SOFTWARE;

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
