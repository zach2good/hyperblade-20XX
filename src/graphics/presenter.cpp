#include "common/compat.h"
#include "graphics/bitmapformat.h"
#include "graphics/presenter.h"
#include "graphics/hudlayer.h"
#include "app/gameapp.h"
#include "app/window.h"
#include "graphics/graphicssettings.h"
#include "graphics/hwrenderer.h"
#include "ui/debugmenu.h"
#include "graphics/prompticons.h"
#include <SDL3/SDL.h>
#include "stb_image.h"
#include <map>
#include <string>
#include <algorithm>

namespace
{

SDL_Renderer* s_pRenderer = nullptr;

SDL_Texture* s_pFrame  = nullptr;
SDL_Texture* s_pHud    = nullptr;
SDL_Texture* s_pCanvas = nullptr;

SDL_Palette* s_pFramePalette = nullptr;
SDL_Palette* s_pHudPalette   = nullptr;

bool s_bBusy = false;

std::vector<Presenter::CanvasIcon>  s_canvasIcons;
std::map<std::string, SDL_Texture*> s_iconTextures;

// Each picture is decoded once, at its full size, so it stays sharp however large the canvas is drawn.
SDL_Texture* IconTexture(const char* pszName)
{
    const auto it = s_iconTextures.find(pszName);

    if (it != s_iconTextures.end())
    {
        return it->second;
    }

    SDL_Texture* pTexture = nullptr;

    const auto png = PromptIcons::Find(pszName);

    auto nWidth    = 0;
    auto nHeight   = 0;
    auto nChannels = 0;

    stbi_uc* const pPixels = [&]() -> stbi_uc*
    {
        if (png.empty())
        {
            return nullptr;
        }

        stbi_set_flip_vertically_on_load_thread(0);

        return stbi_load_from_memory(png.data(), (int)png.size(), &nWidth, &nHeight, &nChannels, 4);
    }();

    if (pPixels != nullptr)
    {
        pTexture = SDL_CreateTexture(s_pRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, nWidth, nHeight);

        if (pTexture != nullptr)
        {
            SDL_UpdateTexture(pTexture, nullptr, pPixels, nWidth * 4);
            SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(pTexture, SDL_SCALEMODE_LINEAR);
        }

        stbi_image_free(pPixels);
    }

    s_iconTextures[pszName] = pTexture;

    return pTexture;
}

class BusyScope
{
public:
    BusyScope()
    {
        s_bBusy = true;
    }

    ~BusyScope()
    {
        s_bBusy = false;
    }

    BusyScope(const BusyScope&)            = delete;
    BusyScope& operator=(const BusyScope&) = delete;
};

// Keeps a streaming texture at the asked size and format, recreating it when either changes.
SDL_Texture* Fit(SDL_Texture* pTexture, SDL_PixelFormat nFormat, int nWidth, int nHeight, SDL_ScaleMode nScale)
{
    if (pTexture != nullptr && pTexture->w == nWidth && pTexture->h == nHeight && pTexture->format == nFormat)
    {
        return pTexture;
    }

    if (pTexture != nullptr)
    {
        SDL_DestroyTexture(pTexture);
    }

    pTexture = SDL_CreateTexture(s_pRenderer, nFormat, SDL_TEXTUREACCESS_STREAMING, nWidth, nHeight);

    if (pTexture != nullptr)
    {
        SDL_SetTextureScaleMode(pTexture, nScale);
    }

    return pTexture;
}

// The largest rect of the source's shape that fits the output, centred; never larger than the source.
SDL_FRect FitRect(int nWidth, int nHeight, bool bUpscale)
{
    int nOutWidth  = 0;
    int nOutHeight = 0;

    SDL_GetCurrentRenderOutputSize(s_pRenderer, &nOutWidth, &nOutHeight);

    auto flScale = (std::min)((float)nOutWidth / nWidth, (float)nOutHeight / nHeight);

    if (bUpscale == false)
    {
        flScale = (std::min)(flScale, 1.0f);
    }

    const auto flWidth  = nWidth * flScale;
    const auto flHeight = nHeight * flScale;

    return SDL_FRect{ (nOutWidth - flWidth) / 2, (nOutHeight - flHeight) / 2, flWidth, flHeight };
}

void SetColours(SDL_Palette* pPalette, const RgbQuad* pColours, int nTransparent)
{
    SDL_Color aColours[256];

    for (int i = 0; i < 256; i++)
    {
        aColours[i] = SDL_Color{ pColours[i].rgbRed, pColours[i].rgbGreen, pColours[i].rgbBlue, 0xff };
    }

    if (nTransparent >= 0)
    {
        aColours[nTransparent].a = 0;
    }

    SDL_SetPaletteColors(pPalette, aColours, 0, 256);
}

// The GPU renderer, taking the shader formats the hardware renderer is built with.
SDL_Renderer* CreateGPURenderer(SDL_Window* pWindow)
{
    const auto props = SDL_CreateProperties();

    SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, "gpu");
    SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, pWindow);
    SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN, true);

    SDL_Renderer* const pRenderer = SDL_CreateRendererWithProperties(props);

    SDL_DestroyProperties(props);

    return pRenderer;
}

// Makes the renderer on first use, on the window SDL already owns: on a GPU device where there is
// one, and on whatever SDL can make otherwise, such as WebGL in a browser.
bool Ready()
{
    if (s_pRenderer != nullptr)
    {
        return true;
    }

    auto* const pWindow = Window::Native().As<SDL_Window>();

    s_pRenderer = CreateGPURenderer(pWindow);

    if (s_pRenderer == nullptr)
    {
        s_pRenderer = SDL_CreateRenderer(pWindow, nullptr);
    }

    if (s_pRenderer == nullptr)
    {
        g_gameApp.Fatal("SDL could not make a renderer: %s", SDL_GetError());

        return false;
    }

    s_pFramePalette = SDL_CreatePalette(256);
    s_pHudPalette   = SDL_CreatePalette(256);

    return true;
}

} // namespace

void Presenter::Frame(const unsigned char* pBits, int nPitch, int nWidth, int nHeight, const RgbQuad* pColours)
{
    const auto busy = BusyScope();

    if (pBits == nullptr || nWidth <= 0 || nHeight <= 0 || Ready() == false)
    {
        return;
    }

    // The hardware renderer draws the scene at the back buffer's size, so the HUD lines up the same.
    SDL_Texture* const pScene = [&]() -> SDL_Texture*
    {
        if (GraphicsSettings::Active() == GraphicsSettings::Renderer::HARDWARE)
        {
            return HwRenderer::Render(HwRenderer::Renderer{ s_pRenderer }, nWidth, nHeight, (const unsigned char*)pColours).As<SDL_Texture>();
        }

        s_pFrame = Fit(s_pFrame, SDL_PIXELFORMAT_INDEX8, nWidth, nHeight, SDL_SCALEMODE_LINEAR);

        if (s_pFrame == nullptr)
        {
            return nullptr;
        }

        SetColours(s_pFramePalette, pColours, -1);
        SDL_SetTexturePalette(s_pFrame, s_pFramePalette);
        SDL_UpdateTexture(s_pFrame, nullptr, pBits, nPitch);

        return s_pFrame;
    }();

    if (pScene == nullptr)
    {
        return;
    }

    const auto rcFrame = FitRect(nWidth, nHeight, false);

    SDL_SetRenderDrawColor(s_pRenderer, 0, 0, 0, 0xff);
    SDL_RenderClear(s_pRenderer);
    SDL_RenderTexture(s_pRenderer, pScene, nullptr, &rcFrame);

    if (HudLayer::TakeFrame())
    {
        const auto& layer = HudLayer::Pixels();

        s_pHud = Fit(s_pHud, SDL_PIXELFORMAT_INDEX8, layer.nWidth, layer.nHeight, SDL_SCALEMODE_NEAREST);

        if (s_pHud != nullptr)
        {
            SetColours(s_pHudPalette, pColours, HudLayer::KEY_INDEX);
            SDL_SetTexturePalette(s_pHud, s_pHudPalette);
            SDL_SetTextureBlendMode(s_pHud, SDL_BLENDMODE_BLEND);
            SDL_UpdateTexture(s_pHud, nullptr, layer.pBits, layer.nPitch);

            // The layer covers the back buffer at its scale, from its origin, as the frame is drawn.
            const auto flToScreen = rcFrame.w / nWidth;

            const auto rcHud = SDL_FRect{ rcFrame.x + layer.nOriginX * flToScreen,
                                          rcFrame.y + layer.nOriginY * flToScreen,
                                          (float)layer.nWidth * layer.nScale * flToScreen,
                                          (float)layer.nHeight * layer.nScale * flToScreen };

            SDL_RenderTexture(s_pRenderer, s_pHud, nullptr, &rcHud);
        }
    }

    DebugMenu::Draw(DebugMenu::Renderer{ s_pRenderer });

    SDL_RenderPresent(s_pRenderer);
}

void Presenter::Canvas(const void* pBits, int nPitch, int nWidth, int nHeight)
{
    const auto busy = BusyScope();

    if (pBits == nullptr || Ready() == false)
    {
        return;
    }

    s_pCanvas = Fit(s_pCanvas, SDL_PIXELFORMAT_XRGB8888, nWidth, nHeight, SDL_SCALEMODE_LINEAR);

    if (s_pCanvas == nullptr)
    {
        return;
    }

    SDL_UpdateTexture(s_pCanvas, nullptr, pBits, nPitch);

    const auto rcCanvas = FitRect(nWidth, nHeight, true);

    SDL_SetRenderDrawColor(s_pRenderer, 0, 0, 0, 0xff);
    SDL_RenderClear(s_pRenderer);
    SDL_RenderTexture(s_pRenderer, s_pCanvas, nullptr, &rcCanvas);

    const auto flScale = rcCanvas.w / nWidth;

    for (const auto& icon : s_canvasIcons)
    {
        SDL_Texture* const pIcon = IconTexture(icon.pszName);

        if (pIcon == nullptr)
        {
            continue;
        }

        const auto rcIcon = SDL_FRect{ rcCanvas.x + icon.nX * flScale, rcCanvas.y + icon.nY * flScale, icon.nSize * flScale, icon.nSize * flScale };

        SDL_RenderTexture(s_pRenderer, pIcon, nullptr, &rcIcon);
    }

    DebugMenu::Draw(DebugMenu::Renderer{ s_pRenderer });

    SDL_RenderPresent(s_pRenderer);
}

bool Presenter::HasGPU()
{
    return Ready() && SDL_GetGPURendererDevice(s_pRenderer) != nullptr;
}

bool Presenter::Busy()
{
    return s_bBusy;
}

void Presenter::SetCanvasIcons(std::vector<CanvasIcon> icons)
{
    s_canvasIcons = std::move(icons);
}
