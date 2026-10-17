#include "graphics/hwtexture.h"
#include "common/path.h"
#include "graphics/texcacheentry.h"
#include "app/gameapp.h"
#include "common/pathstring.h"
#include "stb_image.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <bit>
#include <cctype>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>

namespace
{

constexpr auto WHITE   = 0xffffffffu;
constexpr auto MAGENTA = 0xffff00ffu;

SDL_GPUTexture* s_pWhite   = nullptr;
SDL_GPUTexture* s_pMagenta = nullptr;

// Keyed by the lower-cased name, as ModelDatabase::FindOrLoadTexture matches names.
std::unordered_map<std::string, SDL_GPUTexture*> s_textures;

// Uploads RGBA rows, top first, and fills every mip level below the first.
SDL_GPUTexture* Upload(SDL_GPUDevice* pDevice, const void* pPixels, int nWidth, int nHeight)
{
    const auto nLevels = static_cast<Uint32>(std::bit_width(static_cast<unsigned int>((std::max)(nWidth, nHeight))));
    const auto nBytes  = static_cast<Uint32>(nWidth * nHeight * 4);

    auto info                 = SDL_GPUTextureCreateInfo{};
    info.type                 = SDL_GPU_TEXTURETYPE_2D;
    info.format               = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET; // Mipmaps are blitted.
    info.width                = static_cast<Uint32>(nWidth);
    info.height               = static_cast<Uint32>(nHeight);
    info.layer_count_or_depth = 1;
    info.num_levels           = nLevels;

    SDL_GPUTexture* const pTexture = SDL_CreateGPUTexture(pDevice, &info);

    if (pTexture == nullptr)
    {
        return nullptr;
    }

    auto transferInfo  = SDL_GPUTransferBufferCreateInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size  = nBytes;

    SDL_GPUTransferBuffer* const pTransfer = SDL_CreateGPUTransferBuffer(pDevice, &transferInfo);
    SDL_GPUCommandBuffer* const  pCommands = SDL_AcquireGPUCommandBuffer(pDevice);

    if (pTransfer == nullptr || pCommands == nullptr)
    {
        SDL_ReleaseGPUTransferBuffer(pDevice, pTransfer);
        SDL_ReleaseGPUTexture(pDevice, pTexture);

        if (pCommands != nullptr)
        {
            SDL_CancelGPUCommandBuffer(pCommands);
        }

        return nullptr;
    }

    std::memcpy(SDL_MapGPUTransferBuffer(pDevice, pTransfer, false), pPixels, nBytes);
    SDL_UnmapGPUTransferBuffer(pDevice, pTransfer);

    auto source            = SDL_GPUTextureTransferInfo{};
    source.transfer_buffer = pTransfer;

    auto destination    = SDL_GPUTextureRegion{};
    destination.texture = pTexture;
    destination.w       = static_cast<Uint32>(nWidth);
    destination.h       = static_cast<Uint32>(nHeight);
    destination.d       = 1;

    SDL_GPUCopyPass* const pCopy = SDL_BeginGPUCopyPass(pCommands);

    SDL_UploadToGPUTexture(pCopy, &source, &destination, false);
    SDL_EndGPUCopyPass(pCopy);

    if (nLevels > 1)
    {
        SDL_GenerateMipmapsForGPUTexture(pCommands, pTexture);
    }

    // Submitted ahead of the frame's own commands, so the texture is ready before it is drawn.
    SDL_SubmitGPUCommandBuffer(pCommands);
    SDL_ReleaseGPUTransferBuffer(pDevice, pTransfer);

    return pTexture;
}

SDL_GPUTexture* Solid(SDL_GPUDevice* pDevice, SDL_GPUTexture*& pTexture, unsigned int nABGR)
{
    if (pTexture == nullptr)
    {
        pTexture = Upload(pDevice, &nABGR, 1, 1);
    }

    return pTexture;
}

SDL_GPUTexture* Load(SDL_GPUDevice* pDevice, const char* pszName)
{
    const auto path = PathString(MAX_PATH_LENGTH, g_gameApp.m_strTexturesPath.m_str.m_pchData, pszName, g_pszBitmapExtension);

    auto nWidth    = 0;
    auto nHeight   = 0;
    auto nChannels = 0;

    // The original's surfaces took the bitmap's rows bottom up, as the file stores them, and its UVs
    // count from there.
    stbi_set_flip_vertically_on_load_thread(1);

    stbi_uc* const pPixels = stbi_load(Path::Native(path.m_str.m_pchData).c_str(), &nWidth, &nHeight, &nChannels, 4);

    if (pPixels == nullptr)
    {
        SDL_Log("Texture %s did not load: %s", path.m_str.m_pchData, stbi_failure_reason());

        return Solid(pDevice, s_pMagenta, MAGENTA);
    }

    SDL_GPUTexture* const pTexture = Upload(pDevice, pPixels, nWidth, nHeight);

    stbi_image_free(pPixels);

    if (pTexture == nullptr)
    {
        SDL_Log("Texture %s did not upload: %s", path.m_str.m_pchData, SDL_GetError());

        return Solid(pDevice, s_pMagenta, MAGENTA);
    }

    return pTexture;
}

} // namespace

HwTexture::Texture HwTexture::Find(HwPipeline::Device device, const TexCacheEntry* pEntry)
{
    SDL_GPUDevice* const pDevice = device.As<SDL_GPUDevice>();

    if (pEntry == nullptr || pEntry->m_pszName == nullptr)
    {
        return Texture{ Solid(pDevice, s_pWhite, WHITE) };
    }

    auto key = std::string(pEntry->m_pszName);

    for (auto& c : key)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    const auto it = s_textures.find(key);

    if (it != s_textures.end())
    {
        return Texture{ it->second };
    }

    SDL_GPUTexture* const pTexture = Load(pDevice, pEntry->m_pszName);

    s_textures.emplace(std::move(key), pTexture);

    return Texture{ pTexture };
}
