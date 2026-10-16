#include "graphics/hwrenderer.h"
#include "graphics/graphicssettings.h"
#include "graphics/hwmesh.h"
#include "graphics/hwpipeline.h"
#include "graphics/colorpalette.h"
#include "globals.h"
#include <SDL3/SDL.h>
#include <cstring>
#include <map>
#include <utility>
#include <vector>

namespace
{

// The colour the scene clears to behind the arena.
constexpr auto CLEAR_COLOUR = SDL_FColor{ 0.0f, 0.0f, 0.0f, 1.0f };

// A frame draws a few thousand blocks; more than this means the list is not being consumed.
constexpr auto MAX_DRAWS = 16384u;

// HwMesh::Build winds front faces clockwise as seen on screen, as the original's D3DCULL_CCW drew.
constexpr auto FRONT_CLOCKWISE = true;

struct GPUMesh
{
    SDL_GPUBuffer* pBuffer      = nullptr;
    Uint32         nVertexCount = 0;
};

using MeshKey = std::pair<const MeshBlock*, const MeshMaterial*>;

SDL_GPUDevice*           s_pDevice   = nullptr;
SDL_GPUGraphicsPipeline* s_pPipeline = nullptr;
SDL_GPUTexture*          s_pTarget   = nullptr;
SDL_GPUTexture*          s_pDepth    = nullptr;
SDL_Texture*             s_pTexture  = nullptr;
int                      s_nWidth    = 0;
int                      s_nHeight   = 0;

std::vector<HwRenderer::Draw> s_draws;
std::map<MeshKey, GPUMesh>    s_meshes;

void ReleaseTargets()
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

    if (s_pDepth != nullptr)
    {
        SDL_ReleaseGPUTexture(s_pDevice, s_pDepth);
        s_pDepth = nullptr;
    }
}

SDL_GPUTexture* CreateTarget(SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage, int nWidth, int nHeight)
{
    auto info                 = SDL_GPUTextureCreateInfo{};
    info.type                 = SDL_GPU_TEXTURETYPE_2D;
    info.format               = format;
    info.usage                = usage;
    info.width                = (Uint32)nWidth;
    info.height               = (Uint32)nHeight;
    info.layer_count_or_depth = 1;
    info.num_levels           = 1;

    return SDL_CreateGPUTexture(s_pDevice, &info);
}

// Keeps the colour and depth targets at the asked size, with a renderer texture over the colour.
bool Fit(SDL_Renderer* pRenderer, int nWidth, int nHeight)
{
    if (s_pTarget != nullptr && nWidth == s_nWidth && nHeight == s_nHeight)
    {
        return true;
    }

    ReleaseTargets();

    const auto device = HwPipeline::Device{ s_pDevice };

    s_pTarget = CreateTarget((SDL_GPUTextureFormat)HwPipeline::ColourFormat(), SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER, nWidth, nHeight);
    s_pDepth  = CreateTarget((SDL_GPUTextureFormat)HwPipeline::DepthFormat(device), SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, nWidth, nHeight);

    if (s_pTarget == nullptr || s_pDepth == nullptr)
    {
        ReleaseTargets();

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
        ReleaseTargets();

        return false;
    }

    s_nWidth  = nWidth;
    s_nHeight = nHeight;

    return true;
}

bool Ready(SDL_Renderer* pRenderer)
{
    if (s_pDevice == nullptr)
    {
        s_pDevice = SDL_GetGPURendererDevice(pRenderer);
    }

    if (s_pDevice == nullptr)
    {
        return false;
    }

    if (s_pPipeline == nullptr)
    {
        s_pPipeline = HwPipeline::CreateMeshPipeline(HwPipeline::Device{ s_pDevice }, FRONT_CLOCKWISE).As<SDL_GPUGraphicsPipeline>();
    }

    return s_pPipeline != nullptr;
}

// Builds and uploads every mesh the frame draws that is not on the GPU yet, in one copy pass.
void UploadMeshes(SDL_GPUCommandBuffer* pCommands)
{
    SDL_GPUCopyPass* pCopy = nullptr;

    for (const auto& draw : s_draws)
    {
        const auto key = MeshKey{ draw.pBlock, draw.pMaterials };

        if (s_meshes.contains(key))
        {
            continue;
        }

        auto& mesh = s_meshes[key];

        const auto vertices = HwMesh::Build(*draw.pBlock, draw.pMaterials, draw.nMaterialCount);

        if (vertices.empty())
        {
            continue;
        }

        const auto nBytes = (Uint32)(vertices.size() * sizeof(HwMesh::Vertex));

        auto bufferInfo  = SDL_GPUBufferCreateInfo{};
        bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bufferInfo.size  = nBytes;

        auto transferInfo  = SDL_GPUTransferBufferCreateInfo{};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size  = nBytes;

        SDL_GPUBuffer* const         pBuffer   = SDL_CreateGPUBuffer(s_pDevice, &bufferInfo);
        SDL_GPUTransferBuffer* const pTransfer = SDL_CreateGPUTransferBuffer(s_pDevice, &transferInfo);

        if (pBuffer == nullptr || pTransfer == nullptr)
        {
            SDL_ReleaseGPUBuffer(s_pDevice, pBuffer);
            SDL_ReleaseGPUTransferBuffer(s_pDevice, pTransfer);

            continue;
        }

        std::memcpy(SDL_MapGPUTransferBuffer(s_pDevice, pTransfer, false), vertices.data(), nBytes);
        SDL_UnmapGPUTransferBuffer(s_pDevice, pTransfer);

        if (pCopy == nullptr)
        {
            pCopy = SDL_BeginGPUCopyPass(pCommands);
        }

        const auto source      = SDL_GPUTransferBufferLocation{ pTransfer, 0 };
        const auto destination = SDL_GPUBufferRegion{ pBuffer, 0, nBytes };

        SDL_UploadToGPUBuffer(pCopy, &source, &destination, false);

        // Released now, freed by SDL once the upload has run.
        SDL_ReleaseGPUTransferBuffer(s_pDevice, pTransfer);

        mesh = GPUMesh{ pBuffer, (Uint32)vertices.size() };
    }

    if (pCopy != nullptr)
    {
        SDL_EndGPUCopyPass(pCopy);
    }
}

// Viewport pixels, y down, into clip space, keeping the engine's 0 to 1 depth.
HwPipeline::MeshUniforms ClipUniforms(const HwRenderer::Draw& draw)
{
    const auto flScaleX = 2.0f / (float)draw.nViewportWidth;
    const auto flScaleY = -2.0f / (float)draw.nViewportHeight;

    const float aNDCFromPixel[16] = {
        flScaleX,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        flScaleY,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };

    auto uniforms = HwPipeline::MeshUniforms{};

    for (int nRow = 0; nRow < 4; nRow++)
    {
        for (int nColumn = 0; nColumn < 4; nColumn++)
        {
            auto flSum = 0.0f;

            for (int k = 0; k < 4; k++)
            {
                flSum += aNDCFromPixel[nRow * 4 + k] * draw.aPixelFromModel[k * 4 + nColumn];
            }

            uniforms.aClipFromModel[nRow * 4 + nColumn] = flSum;
        }
    }

    std::memcpy(uniforms.aLightDir, draw.aLightDir, sizeof(uniforms.aLightDir));

    return uniforms;
}

// The shaded models' colours are palette indices, lit along the live palette's ramps.
HwPipeline::ColourUniforms PaletteUniforms(const unsigned char* pPalette)
{
    auto uniforms = HwPipeline::ColourUniforms{};

    uniforms.nPalettised      = 1;
    uniforms.nLargeRampFirst  = g_pColorPalette->m_largeRampBounds.nFirst;
    uniforms.nLargeRampLast   = g_pColorPalette->m_largeRampBounds.nCount;
    uniforms.nSmallRampLength = g_pColorPalette->m_rampLength.nFirst;
    uniforms.nLargeRampLength = g_pColorPalette->m_rampLength.nCount;

    for (int i = 0; i < 256; i++)
    {
        uniforms.aPalette[i][0] = pPalette[i * 4 + 2] / 255.0f;
        uniforms.aPalette[i][1] = pPalette[i * 4 + 1] / 255.0f;
        uniforms.aPalette[i][2] = pPalette[i * 4 + 0] / 255.0f;
        uniforms.aPalette[i][3] = 1.0f;
    }

    return uniforms;
}

} // namespace

bool HwRenderer::Capturing()
{
    return GraphicsSettings::Active() == GraphicsSettings::Renderer::HARDWARE;
}

void HwRenderer::Submit(const Draw& draw)
{
    if (s_draws.size() < MAX_DRAWS)
    {
        s_draws.push_back(draw);
    }
}

void HwRenderer::ForgetMeshes()
{
    for (const auto& [key, mesh] : s_meshes)
    {
        SDL_ReleaseGPUBuffer(s_pDevice, mesh.pBuffer);
    }

    s_meshes.clear();
}

HwRenderer::Texture HwRenderer::Render(Renderer renderer, int nWidth, int nHeight, const unsigned char* pPalette)
{
    SDL_Renderer* const pRenderer = renderer.As<SDL_Renderer>();

    if (nWidth <= 0 || nHeight <= 0 || !Ready(pRenderer) || !Fit(pRenderer, nWidth, nHeight))
    {
        s_draws.clear();

        return Texture{};
    }

    SDL_GPUCommandBuffer* const pCommands = SDL_AcquireGPUCommandBuffer(s_pDevice);

    if (pCommands == nullptr)
    {
        s_draws.clear();

        return Texture{};
    }

    UploadMeshes(pCommands);

    auto target        = SDL_GPUColorTargetInfo{};
    target.texture     = s_pTarget;
    target.clear_color = CLEAR_COLOUR;
    target.load_op     = SDL_GPU_LOADOP_CLEAR;
    target.store_op    = SDL_GPU_STOREOP_STORE;

    auto depth             = SDL_GPUDepthStencilTargetInfo{};
    depth.texture          = s_pDepth;
    depth.clear_depth      = 1.0f;
    depth.load_op          = SDL_GPU_LOADOP_CLEAR;
    depth.store_op         = SDL_GPU_STOREOP_DONT_CARE;
    depth.stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE;
    depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    const auto colours = PaletteUniforms(pPalette);

    SDL_GPURenderPass* pPass  = nullptr;
    auto               bArena = false;

    for (const auto& draw : s_draws)
    {
        // A new layer starts a new pass with the depth cleared and the colour kept.
        if (pPass == nullptr || draw.bArena != bArena)
        {
            if (pPass != nullptr)
            {
                SDL_EndGPURenderPass(pPass);

                target.load_op = SDL_GPU_LOADOP_LOAD;
            }

            pPass  = SDL_BeginGPURenderPass(pCommands, &target, 1, &depth);
            bArena = draw.bArena;

            SDL_BindGPUGraphicsPipeline(pPass, s_pPipeline);
            SDL_PushGPUVertexUniformData(pCommands, 1, &colours, sizeof(colours));
        }

        const auto it = s_meshes.find(MeshKey{ draw.pBlock, draw.pMaterials });

        if (it == s_meshes.end() || it->second.pBuffer == nullptr || draw.nViewportWidth <= 0 || draw.nViewportHeight <= 0)
        {
            continue;
        }

        const auto viewport = SDL_GPUViewport{ (float)draw.nViewportX, (float)draw.nViewportY, (float)draw.nViewportWidth, (float)draw.nViewportHeight, 0.0f, 1.0f };
        const auto binding  = SDL_GPUBufferBinding{ it->second.pBuffer, 0 };
        const auto uniforms = ClipUniforms(draw);

        SDL_SetGPUViewport(pPass, &viewport);
        SDL_BindGPUVertexBuffers(pPass, 0, &binding, 1);
        SDL_PushGPUVertexUniformData(pCommands, 0, &uniforms, sizeof(uniforms));
        SDL_DrawGPUPrimitives(pPass, it->second.nVertexCount, 1, 0, 0);
    }

    // A frame with nothing to draw still clears.
    if (pPass == nullptr)
    {
        pPass = SDL_BeginGPURenderPass(pCommands, &target, 1, &depth);
    }

    SDL_EndGPURenderPass(pPass);
    SDL_SubmitGPUCommandBuffer(pCommands);

    s_draws.clear();

    return Texture{ s_pTexture };
}
