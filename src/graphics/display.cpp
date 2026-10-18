#include "common/compat.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "renderer.h"
#include "ui/infobarbase.h"
struct GameApp;
#include "app/window.h"
#include "app/wndproc.h"
#include "graphics/graphics.h"
#include "app/menuids.h"
#include <cstdlib>
#include "app/gameapp.h"
#include "graphics/display.h"
#include "softrenderer.h"

unsigned int g_dwDisplayWidth  = 640;
unsigned int g_dwDisplayHeight = 480;

// The back buffer resize result.
// FUNCTION: HYPERX 0x0044b500
int Display::Initialize()
{
    Window::ApplyFullscreenStyle();

    g_cMdmTexLoaded = (char)Graphics::LoadMdmTexDLL();
    Display::ApplyRenderOptions(g_cMdmTexLoaded & 0xff, 0);

    int nWidth  = g_dwDisplayWidth;
    int nHeight = g_dwDisplayHeight;

    g_bBackBufferReady = SoftRenderer::ResizeBackBuffer(&nWidth, &nHeight);

    if (g_bFullScreen == 0)
    {
        WndProc::OnResize((int)g_dwDisplayWidth, (int)g_dwDisplayHeight);
    }

    if (g_bBackBufferReady != 0 && g_pRenderer != nullptr)
    {
        g_pRenderer->LayoutSurfaces();
    }

    return g_bBackBufferReady;
}

// FUNCTION: HYPERX 0x0044b850
int Display::ChangeColorDepth(int)
{
    // The game runs framed on the desktop and never changes the display mode.
    return 0;
}

// FUNCTION: HYPERX 0x00493900
void Display::ApplyRenderOptions(int bTexEnabled, int bBilinearFilter)
{
    GameApp::SetModelPath(bTexEnabled);

    Renderer::SetRenderOption(RENDER_OPTION_TEXTURED, bTexEnabled);
    Renderer::SetRenderOption(RENDER_OPTION_PERSPECTIVE, bTexEnabled);
    Renderer::SetRenderOption(RENDER_OPTION_FILTERED, bBilinearFilter);

    g_bPerspectiveTexturing = bTexEnabled;
    g_bBilinearFilter       = bBilinearFilter;
}
