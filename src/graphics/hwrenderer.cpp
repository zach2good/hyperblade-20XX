#include "graphics/hwrenderer.h"
#include <SDL3/SDL.h>

namespace
{

// The colour the scene clears to until it draws anything.
constexpr auto CLEAR_COLOUR = SDL_FColor{ 0.05f, 0.12f, 0.16f, 1.0f };

SDL_GPUDevice*  s_pDevice  = nullptr;
SDL_GPUTexture* s_pTarget  = nullptr;
SDL_Texture*    s_pTexture = nullptr;
int             s_nWidth   = 0;
int             s_nHeight  = 0;

void Release()
{
    if (s_pTexture != nullptr)
    {
        SDL_DestroyTexture(s_pTexture);
        s_pTexture = nullptr;
    }

    if (s_pTarget != nullptr)
    {
        SDL_ReleaseGPUTexture(s_pDevice, s_pTarget);
        s_pTarget = nullptr;
    }
}

// Keeps the colour target at the asked size, with a renderer texture over the same GPU texture.
bool Fit(SDL_Renderer* pRenderer, int nWidth, int nHeight)
{
    if (s_pTarget != nullptr && nWidth == s_nWidth && nHeight == s_nHeight)
    {
        return true;
    }

    Release();

    s_pDevice = SDL_GetGPURendererDevice(pRenderer);

    if (s_pDevice == nullptr)
    {
        return false;
    }

    auto info                 = SDL_GPUTextureCreateInfo{};
    info.type                 = SDL_GPU_TEXTURETYPE_2D;
    info.format               = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage                = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width                = (Uint32)nWidth;
    info.height               = (Uint32)nHeight;
    info.layer_count_or_depth = 1;
    info.num_levels           = 1;

    s_pTarget = SDL_CreateGPUTexture(s_pDevice, &info);

    if (s_pTarget == nullptr)
    {
        return false;
    }

    const auto props = SDL_CreateProperties();

    SDL_SetPointerProperty(props, SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_POINTER, s_pTarget);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_ABGR8888);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STATIC);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, nWidth);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, nHeight);

    s_pTexture = SDL_CreateTextureWithProperties(pRenderer, props);

    SDL_DestroyProperties(props);

    if (s_pTexture == nullptr)
    {
        Release();

        return false;
    }

    s_nWidth  = nWidth;
    s_nHeight = nHeight;

    return true;
}

} // namespace

HwRenderer::Texture HwRenderer::Render(Renderer renderer, int nWidth, int nHeight)
{
    if (nWidth <= 0 || nHeight <= 0 || !Fit(renderer.As<SDL_Renderer>(), nWidth, nHeight))
    {
        return Texture{};
    }

    SDL_GPUCommandBuffer* const pCommands = SDL_AcquireGPUCommandBuffer(s_pDevice);

    if (pCommands == nullptr)
    {
        return Texture{};
    }

    auto target        = SDL_GPUColorTargetInfo{};
    target.texture     = s_pTarget;
    target.clear_color = CLEAR_COLOUR;
    target.load_op     = SDL_GPU_LOADOP_CLEAR;
    target.store_op    = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* const pPass = SDL_BeginGPURenderPass(pCommands, &target, 1, nullptr);

    SDL_EndGPURenderPass(pPass);
    SDL_SubmitGPUCommandBuffer(pCommands);

    return Texture{ s_pTexture };
}
