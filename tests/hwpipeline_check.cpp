// Draws a lit triangle through the mesh pipeline on a real GPU and reads the pixels back.

#include "graphics/hwmesh.h"
#include "graphics/hwpipeline.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{

constexpr auto SIZE = 32;

constexpr auto CLEAR_COLOUR = SDL_FColor{ 0.0f, 1.0f, 0.0f, 1.0f };

int s_nFailed = 0;

void Expect(bool bOK, const char* pszWhat)
{
    if (!bOK)
    {
        std::printf("failed: %s\n", pszWhat);
        s_nFailed++;
    }
}

bool Near(const unsigned char* pPixel, int nRed, int nGreen, int nBlue)
{
    return std::abs(pPixel[0] - nRed) <= 1 && std::abs(pPixel[1] - nGreen) <= 1 && std::abs(pPixel[2] - nBlue) <= 1 && pPixel[3] == 255;
}

HwMesh::Vertex Corner(float x, float y)
{
    auto vertex         = HwMesh::Vertex{};
    vertex.aPosition[0] = x;
    vertex.aPosition[1] = y;
    vertex.aPosition[2] = 0.5f;
    vertex.aNormal[2]   = 1.0f;
    vertex.nColour      = 0xffc86432;
    vertex.flDiffuse    = 0.5f;
    vertex.flAmbient    = 0.25f;

    return vertex;
}

struct Scene
{
    SDL_GPUDevice*           pDevice;
    SDL_GPUGraphicsPipeline* pPipeline;
    SDL_GPUTexture*          pColour;
    SDL_GPUTexture*          pDepth;
    SDL_GPUBuffer*           pVertices;
    SDL_GPUTransferBuffer*   pDownload;
};

// Draws three vertices from nFirst and leaves the colour target's pixels in the download buffer.
void Draw(const Scene& scene, Uint32 nFirst)
{
    SDL_GPUCommandBuffer* const pCommands = SDL_AcquireGPUCommandBuffer(scene.pDevice);

    auto colour        = SDL_GPUColorTargetInfo{};
    colour.texture     = scene.pColour;
    colour.clear_color = CLEAR_COLOUR;
    colour.load_op     = SDL_GPU_LOADOP_CLEAR;
    colour.store_op    = SDL_GPU_STOREOP_STORE;

    auto depth             = SDL_GPUDepthStencilTargetInfo{};
    depth.texture          = scene.pDepth;
    depth.clear_depth      = 1.0f;
    depth.load_op          = SDL_GPU_LOADOP_CLEAR;
    depth.store_op         = SDL_GPU_STOREOP_DONT_CARE;
    depth.stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE;
    depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    // The identity puts the model straight into clip space, lit head on.
    auto uniforms = HwPipeline::MeshUniforms{};

    for (int i = 0; i < 4; i++)
    {
        uniforms.aClipFromModel[i * 5] = 1.0f;
    }

    uniforms.aLightDir[2] = 1.0f;

    SDL_PushGPUVertexUniformData(pCommands, 0, &uniforms, sizeof(uniforms));

    const auto colours = HwPipeline::ColourUniforms{};

    SDL_PushGPUVertexUniformData(pCommands, 1, &colours, sizeof(colours));

    SDL_GPURenderPass* const pPass = SDL_BeginGPURenderPass(pCommands, &colour, 1, &depth);

    const auto binding = SDL_GPUBufferBinding{ scene.pVertices, 0 };

    SDL_BindGPUGraphicsPipeline(pPass, scene.pPipeline);
    SDL_BindGPUVertexBuffers(pPass, 0, &binding, 1);
    SDL_DrawGPUPrimitives(pPass, 3, 1, nFirst, 0);
    SDL_EndGPURenderPass(pPass);

    SDL_GPUCopyPass* const pCopy = SDL_BeginGPUCopyPass(pCommands);

    auto region    = SDL_GPUTextureRegion{};
    region.texture = scene.pColour;
    region.w       = SIZE;
    region.h       = SIZE;
    region.d       = 1;

    auto transfer            = SDL_GPUTextureTransferInfo{};
    transfer.transfer_buffer = scene.pDownload;

    SDL_DownloadFromGPUTexture(pCopy, &region, &transfer);
    SDL_EndGPUCopyPass(pCopy);

    SDL_GPUFence* const pFence = SDL_SubmitGPUCommandBufferAndAcquireFence(pCommands);

    SDL_WaitForGPUFences(scene.pDevice, true, &pFence, 1);
    SDL_ReleaseGPUFence(scene.pDevice, pFence);
}

void Check(const Scene& scene)
{
    // Clockwise from the bottom left, with clip space y up, then the same corners reversed.
    const HwMesh::Vertex aVertices[] = {
        Corner(-0.75f, -0.75f),
        Corner(0.0f, 0.75f),
        Corner(0.75f, -0.75f),
        Corner(-0.75f, -0.75f),
        Corner(0.75f, -0.75f),
        Corner(0.0f, 0.75f),
    };

    SDL_GPUTransferBuffer* const pUpload = [&]
    {
        auto info  = SDL_GPUTransferBufferCreateInfo{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size  = sizeof(aVertices);

        return SDL_CreateGPUTransferBuffer(scene.pDevice, &info);
    }();

    std::memcpy(SDL_MapGPUTransferBuffer(scene.pDevice, pUpload, false), aVertices, sizeof(aVertices));
    SDL_UnmapGPUTransferBuffer(scene.pDevice, pUpload);

    SDL_GPUCommandBuffer* const pCommands = SDL_AcquireGPUCommandBuffer(scene.pDevice);
    SDL_GPUCopyPass* const      pCopy     = SDL_BeginGPUCopyPass(pCommands);

    const auto source      = SDL_GPUTransferBufferLocation{ pUpload, 0 };
    const auto destination = SDL_GPUBufferRegion{ scene.pVertices, 0, sizeof(aVertices) };

    SDL_UploadToGPUBuffer(pCopy, &source, &destination, false);
    SDL_EndGPUCopyPass(pCopy);
    SDL_SubmitGPUCommandBuffer(pCommands);
    SDL_ReleaseGPUTransferBuffer(scene.pDevice, pUpload);

    const auto nCentre  = (SIZE / 2 * SIZE + SIZE / 2) * 4;
    const auto nOutside = (SIZE + 1) * 4;

    Draw(scene, 0);

    {
        const auto* const pPixels = (const unsigned char*)SDL_MapGPUTransferBuffer(scene.pDevice, scene.pDownload, false);

        // Lit at 1 * 0.5 + 0.25, each channel truncated as D3DRend::DrawFace did.
        Expect(Near(pPixels + nCentre, 150, 75, 37), "centre lit");
        Expect(Near(pPixels + nOutside, 0, 255, 0), "outside cleared");

        SDL_UnmapGPUTransferBuffer(scene.pDevice, scene.pDownload);
    }

    Draw(scene, 3);

    {
        const auto* const pPixels = (const unsigned char*)SDL_MapGPUTransferBuffer(scene.pDevice, scene.pDownload, false);

        Expect(Near(pPixels + nCentre, 0, 255, 0), "reversed winding culled");

        SDL_UnmapGPUTransferBuffer(scene.pDevice, scene.pDownload);
    }
}

SDL_GPUTexture* CreateTarget(SDL_GPUDevice* pDevice, unsigned int nFormat, SDL_GPUTextureUsageFlags usage)
{
    auto info                 = SDL_GPUTextureCreateInfo{};
    info.type                 = SDL_GPU_TEXTURETYPE_2D;
    info.format               = (SDL_GPUTextureFormat)nFormat;
    info.usage                = usage;
    info.width                = SIZE;
    info.height               = SIZE;
    info.layer_count_or_depth = 1;
    info.num_levels           = 1;

    return SDL_CreateGPUTexture(pDevice, &info);
}

} // namespace

int main()
{
    // The GPU API wants the video subsystem even with no window.
    SDL_GPUDevice* const pDevice = [&]
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            return (SDL_GPUDevice*)nullptr;
        }

        return SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);
    }();

    if (pDevice == nullptr)
    {
        std::printf("no GPU device, skipped (%s)\n", SDL_GetError());
        SDL_Quit();

        return 0;
    }

    std::printf("driver %s\n", SDL_GetGPUDeviceDriver(pDevice));

    const auto device   = HwPipeline::Device{ pDevice };
    const auto pipeline = HwPipeline::CreateMeshPipeline(device, true);

    auto scene      = Scene{};
    scene.pDevice   = pDevice;
    scene.pPipeline = pipeline.As<SDL_GPUGraphicsPipeline>();
    scene.pColour   = CreateTarget(pDevice, HwPipeline::ColourFormat(), SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);
    scene.pDepth    = CreateTarget(pDevice, HwPipeline::DepthFormat(device), SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);

    scene.pVertices = [&]
    {
        auto info  = SDL_GPUBufferCreateInfo{};
        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size  = sizeof(HwMesh::Vertex) * 6;

        return SDL_CreateGPUBuffer(pDevice, &info);
    }();

    scene.pDownload = [&]
    {
        auto info  = SDL_GPUTransferBufferCreateInfo{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
        info.size  = SIZE * SIZE * 4;

        return SDL_CreateGPUTransferBuffer(pDevice, &info);
    }();

    Expect(pipeline && scene.pColour != nullptr && scene.pDepth != nullptr && scene.pVertices != nullptr && scene.pDownload != nullptr, "GPU objects created");

    if (s_nFailed == 0)
    {
        Check(scene);
    }

    SDL_ReleaseGPUTransferBuffer(pDevice, scene.pDownload);
    SDL_ReleaseGPUBuffer(pDevice, scene.pVertices);
    SDL_ReleaseGPUTexture(pDevice, scene.pDepth);
    SDL_ReleaseGPUTexture(pDevice, scene.pColour);
    HwPipeline::Release(device, pipeline);
    SDL_DestroyGPUDevice(pDevice);
    SDL_Quit();

    std::printf("%d failed\n", s_nFailed);

    if (s_nFailed != 0)
    {
        return 1;
    }

    return 0;
}
