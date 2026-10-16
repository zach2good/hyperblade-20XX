#include "graphics/hwrenderer.h"
#include "graphics/graphicssettings.h"
#include "graphics/hwmesh.h"
#include "graphics/hwpipeline.h"
#include "graphics/hwtexture.h"
#include "graphics/colorpalette.h"
#include "graphics/graphicsglobals.h"
#include "globals.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
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

// ponytail: one map a light covers the whole arena; cascades if close shadows look soft.
constexpr auto SHADOW_SIZE = 4096;

struct GPURange
{
    Uint32          nFirst   = 0;
    Uint32          nCount   = 0;
    SDL_GPUTexture* pTexture = nullptr; // Null for untextured faces.
};

struct GPUMesh
{
    SDL_GPUBuffer*        pBuffer = nullptr;
    std::vector<GPURange> ranges;
    Uint32                nVertexCount = 0;
    float                 flRadius     = 0.0f; // The farthest vertex from the bone's origin.
};

using MeshKey = std::pair<const MeshBlock*, const MeshMaterial*>;

SDL_GPUDevice*           s_pDevice   = nullptr;
SDL_GPUGraphicsPipeline* s_pPipeline = nullptr;
SDL_GPUSampler*          s_pNearest  = nullptr;
SDL_GPUSampler*          s_pLinear   = nullptr;
SDL_GPUTexture*          s_pWhite    = nullptr;
SDL_GPUTexture*          s_pTarget   = nullptr;
SDL_GPUTexture*          s_pDepth    = nullptr;
SDL_Texture*             s_pTexture  = nullptr;
SDL_GPUGraphicsPipeline* s_pShadowPipeline = nullptr;
SDL_GPUSampler*          s_pShadowSampler  = nullptr;
SDL_GPUTexture*          s_apShadowMaps[2] = {};
float                    s_aPointLight[4]  = {};
int                      s_nWidth    = 0;
int                      s_nHeight   = 0;
bool                     s_bTextures = false;
bool                     s_bSmooth   = false;
bool                     s_bLinear   = false;
unsigned int             s_nFrame    = 0;

std::vector<HwRenderer::Draw> s_draws;
std::map<MeshKey, GPUMesh>    s_meshes;

// Vertices made fresh each frame, uploaded together into one buffer that grows as it needs to.
std::vector<HwMesh::Vertex> s_dynamic;
SDL_GPUBuffer*              s_pDynamicBuffer   = nullptr;
Uint32                      s_nDynamicCapacity = 0;

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

    const auto device = HwPipeline::Device{ s_pDevice };

    if (s_pPipeline == nullptr)
    {
        s_pPipeline = HwPipeline::CreateMeshPipeline(device, FRONT_CLOCKWISE).As<SDL_GPUGraphicsPipeline>();
    }

    if (s_pNearest == nullptr)
    {
        s_pNearest = HwPipeline::CreateSampler(device, false).As<SDL_GPUSampler>();
    }

    if (s_pLinear == nullptr)
    {
        s_pLinear = HwPipeline::CreateSampler(device, true).As<SDL_GPUSampler>();
    }

    // Untextured faces sample it, as the shader's texture slot is always bound.
    if (s_pWhite == nullptr)
    {
        s_pWhite = HwTexture::Find(device, nullptr).As<SDL_GPUTexture>();
    }

    if (s_pShadowPipeline == nullptr)
    {
        s_pShadowPipeline = HwPipeline::CreateShadowPipeline(device).As<SDL_GPUGraphicsPipeline>();
    }

    if (s_pShadowSampler == nullptr)
    {
        s_pShadowSampler = HwPipeline::CreateShadowSampler(device).As<SDL_GPUSampler>();
    }

    // Made whether or not shadows are on, as the shader's shadow slots are always bound.
    for (auto& pMap : s_apShadowMaps)
    {
        if (pMap == nullptr)
        {
            pMap = CreateTarget((SDL_GPUTextureFormat)HwPipeline::ShadowFormat(device), SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER, SHADOW_SIZE, SHADOW_SIZE);
        }
    }

    return s_pPipeline != nullptr && s_pNearest != nullptr && s_pLinear != nullptr && s_pWhite != nullptr && s_pShadowPipeline != nullptr && s_pShadowSampler != nullptr && s_apShadowMaps[0] != nullptr && s_apShadowMaps[1] != nullptr;
}

void UploadBuffer(SDL_GPUCopyPass* pCopy, SDL_GPUBuffer* pBuffer, const void* pData, Uint32 nBytes)
{
    auto transferInfo  = SDL_GPUTransferBufferCreateInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size  = nBytes;

    SDL_GPUTransferBuffer* const pTransfer = SDL_CreateGPUTransferBuffer(s_pDevice, &transferInfo);

    if (pTransfer == nullptr)
    {
        return;
    }

    std::memcpy(SDL_MapGPUTransferBuffer(s_pDevice, pTransfer, false), pData, nBytes);
    SDL_UnmapGPUTransferBuffer(s_pDevice, pTransfer);

    const auto source      = SDL_GPUTransferBufferLocation{ pTransfer, 0 };
    const auto destination = SDL_GPUBufferRegion{ pBuffer, 0, nBytes };

    SDL_UploadToGPUBuffer(pCopy, &source, &destination, false);

    // Released now, freed by SDL once the upload has run.
    SDL_ReleaseGPUTransferBuffer(s_pDevice, pTransfer);
}

SDL_GPUBuffer* CreateVertexBuffer(Uint32 nBytes)
{
    auto info  = SDL_GPUBufferCreateInfo{};
    info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    info.size  = nBytes;

    return SDL_CreateGPUBuffer(s_pDevice, &info);
}

void UploadDynamic(SDL_GPUCopyPass* pCopy)
{
    const auto nBytes = (Uint32)(s_dynamic.size() * sizeof(HwMesh::Vertex));

    if (nBytes == 0)
    {
        return;
    }

    if (nBytes > s_nDynamicCapacity)
    {
        SDL_ReleaseGPUBuffer(s_pDevice, s_pDynamicBuffer);

        s_nDynamicCapacity = nBytes * 2;
        s_pDynamicBuffer   = CreateVertexBuffer(s_nDynamicCapacity);
    }

    if (s_pDynamicBuffer != nullptr)
    {
        UploadBuffer(pCopy, s_pDynamicBuffer, s_dynamic.data(), nBytes);
    }
}

// Builds and uploads every mesh the frame draws that is not on the GPU yet, and the frame's own
// vertices, in one copy pass.
void UploadMeshes(SDL_GPUCommandBuffer* pCommands)
{
    SDL_GPUCopyPass* const pCopy = SDL_BeginGPUCopyPass(pCommands);

    UploadDynamic(pCopy);

    for (const auto& draw : s_draws)
    {
        if (draw.pBlock == nullptr)
        {
            continue;
        }

        const auto key = MeshKey{ draw.pBlock, draw.pMaterials };

        if (s_meshes.contains(key))
        {
            continue;
        }

        auto& mesh = s_meshes[key];

        const auto built = HwMesh::Build(*draw.pBlock, draw.pMaterials, draw.nMaterialCount, s_bSmooth);

        if (built.vertices.empty())
        {
            continue;
        }

        const auto nBytes = (Uint32)(built.vertices.size() * sizeof(HwMesh::Vertex));

        SDL_GPUBuffer* const pBuffer = CreateVertexBuffer(nBytes);

        if (pBuffer == nullptr)
        {
            continue;
        }

        UploadBuffer(pCopy, pBuffer, built.vertices.data(), nBytes);

        mesh.pBuffer      = pBuffer;
        mesh.nVertexCount = (Uint32)built.vertices.size();

        for (const auto& vertex : built.vertices)
        {
            const auto& p = vertex.aPosition;

            mesh.flRadius = (std::max)(mesh.flRadius, std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]));
        }

        for (const auto& range : built.ranges)
        {
            // The entry may be freed with its model, so its texture is found now.
            SDL_GPUTexture* const pTexture = [&]() -> SDL_GPUTexture*
            {
                if (range.pTexture == nullptr)
                {
                    return nullptr;
                }

                return HwTexture::Find(HwPipeline::Device{ s_pDevice }, range.pTexture).As<SDL_GPUTexture>();
            }();

            mesh.ranges.push_back(GPURange{ range.nFirst, range.nCount, pTexture });
        }
    }

    SDL_EndGPUCopyPass(pCopy);
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

    std::memcpy(uniforms.aWorldFromModel, draw.aWorldFromModel, sizeof(uniforms.aWorldFromModel));
    std::memcpy(uniforms.aLightDirs, draw.aLightDirs, sizeof(uniforms.aLightDirs));

    uniforms.aFlags[0] = (float)draw.bPalettised;
    uniforms.aFlags[1] = (float)draw.nKeyLight;
    uniforms.aFlags[2] = (float)draw.bArena;

    return uniforms;
}

using Matrix = std::array<float, 16>;

Matrix Multiply(const float* pA, const float* pB)
{
    auto out = Matrix{};

    for (int nRow = 0; nRow < 4; nRow++)
    {
        for (int nColumn = 0; nColumn < 4; nColumn++)
        {
            for (int k = 0; k < 4; k++)
            {
                out[nRow * 4 + nColumn] += pA[nRow * 4 + k] * pB[k * 4 + nColumn];
            }
        }
    }

    return out;
}

const GPUMesh* FindMesh(const HwRenderer::Draw& draw)
{
    if (draw.pBlock == nullptr)
    {
        return nullptr;
    }

    const auto it = s_meshes.find(MeshKey{ draw.pBlock, draw.pMaterials });

    if (it == s_meshes.end() || it->second.pBuffer == nullptr)
    {
        return nullptr;
    }

    return &it->second;
}

// The arena only receives: it is a closed dome, so its roof would shadow the whole floor.
const GPUMesh* FindCaster(const HwRenderer::Draw& draw)
{
    if (draw.bArena)
    {
        return nullptr;
    }

    return FindMesh(draw);
}

// An orthographic view down the light over everything that casts, depth 0 nearest the light.
// With nothing to cast it puts every point past the far plane, which the shader reads as lit.
Matrix LightClipFromWorld(const Vec3& vToLight)
{
    const auto forward = std::array<float, 3>{ -vToLight.x, -vToLight.y, -vToLight.z };

    const auto upAxis = [&]
    {
        if (std::abs(forward[2]) > 0.99f)
        {
            return std::array<float, 3>{ 0.0f, 1.0f, 0.0f };
        }

        return std::array<float, 3>{ 0.0f, 0.0f, 1.0f };
    }();

    auto right = std::array<float, 3>{ forward[1] * upAxis[2] - forward[2] * upAxis[1], forward[2] * upAxis[0] - forward[0] * upAxis[2], forward[0] * upAxis[1] - forward[1] * upAxis[0] };

    const auto flRightLength = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);

    for (auto& fl : right)
    {
        fl /= flRightLength;
    }

    const auto up = std::array<float, 3>{ right[1] * forward[2] - right[2] * forward[1], right[2] * forward[0] - right[0] * forward[2], right[0] * forward[1] - right[1] * forward[0] };

    const auto axes = std::array<const std::array<float, 3>*, 3>{ &right, &up, &forward };

    auto aflMin = std::array<float, 3>{ 1e30f, 1e30f, 1e30f };
    auto aflMax = std::array<float, 3>{ -1e30f, -1e30f, -1e30f };

    for (const auto& draw : s_draws)
    {
        const auto* const pMesh = FindCaster(draw);

        if (pMesh == nullptr)
        {
            continue;
        }

        const auto* const w = draw.aWorldFromModel;

        // The bone's largest axis scales the mesh's reach.
        auto flScale = 0.0f;

        for (int nColumn = 0; nColumn < 3; nColumn++)
        {
            flScale = (std::max)(flScale, std::sqrt(w[nColumn] * w[nColumn] + w[4 + nColumn] * w[4 + nColumn] + w[8 + nColumn] * w[8 + nColumn]));
        }

        const auto flReach = pMesh->flRadius * flScale;

        for (int nAxis = 0; nAxis < 3; nAxis++)
        {
            const auto& axis = *axes[(size_t)nAxis];

            const auto flAt = axis[0] * w[3] + axis[1] * w[7] + axis[2] * w[11];

            aflMin[(size_t)nAxis] = (std::min)(aflMin[(size_t)nAxis], flAt - flReach);
            aflMax[(size_t)nAxis] = (std::max)(aflMax[(size_t)nAxis], flAt + flReach);
        }
    }

    auto out = Matrix{};

    if (aflMin[0] > aflMax[0])
    {
        out[11] = 2.0f;
        out[15] = 1.0f;

        return out;
    }

    for (int nAxis = 0; nAxis < 3; nAxis++)
    {
        const auto& axis    = *axes[(size_t)nAxis];
        const auto  flSpan  = (std::max)(aflMax[(size_t)nAxis] - aflMin[(size_t)nAxis], 1.0f);
        const auto  flScale = [&]
        {
            // Depth runs 0 to 1, the other two -1 to 1.
            if (nAxis == 2)
            {
                return 1.0f / flSpan;
            }

            return 2.0f / flSpan;
        }();

        const auto flOffset = [&]
        {
            if (nAxis == 2)
            {
                return -aflMin[2] * flScale;
            }

            return -(aflMin[(size_t)nAxis] + aflMax[(size_t)nAxis]) * 0.5f * flScale;
        }();

        out[nAxis * 4 + 0] = axis[0] * flScale;
        out[nAxis * 4 + 1] = axis[1] * flScale;
        out[nAxis * 4 + 2] = axis[2] * flScale;
        out[nAxis * 4 + 3] = flOffset;
    }

    out[15] = 1.0f;

    return out;
}

void DrawShadowMap(SDL_GPUCommandBuffer* pCommands, SDL_GPUTexture* pMap, const Matrix& lightClipFromWorld)
{
    auto depth             = SDL_GPUDepthStencilTargetInfo{};
    depth.texture          = pMap;
    depth.clear_depth      = 1.0f;
    depth.load_op          = SDL_GPU_LOADOP_CLEAR;
    depth.store_op         = SDL_GPU_STOREOP_STORE;
    depth.stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE;
    depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* const pPass = SDL_BeginGPURenderPass(pCommands, nullptr, 0, &depth);

    SDL_BindGPUGraphicsPipeline(pPass, s_pShadowPipeline);

    for (const auto& draw : s_draws)
    {
        const auto* const pMesh = FindCaster(draw);

        if (pMesh == nullptr)
        {
            continue;
        }

        const auto product  = Multiply(lightClipFromWorld.data(), draw.aWorldFromModel);
        const auto binding  = SDL_GPUBufferBinding{ pMesh->pBuffer, 0 };
        auto       uniforms = HwPipeline::ShadowUniforms{};

        std::memcpy(uniforms.aLightClipFromModel, product.data(), sizeof(uniforms.aLightClipFromModel));

        SDL_BindGPUVertexBuffers(pPass, 0, &binding, 1);
        SDL_PushGPUVertexUniformData(pCommands, 0, &uniforms, sizeof(uniforms));
        SDL_DrawGPUPrimitives(pPass, pMesh->nVertexCount, 1, 0, 0);
    }

    SDL_EndGPURenderPass(pPass);
}

// The shaded models' colours are palette indices, lit along the live palette's ramps; the
// textured models' are RGB.
HwPipeline::ColourUniforms PaletteUniforms(const unsigned char* pPalette, bool bShadows)
{
    auto uniforms = HwPipeline::ColourUniforms{};

    std::memcpy(uniforms.aPointLight, s_aPointLight, sizeof(uniforms.aPointLight));

    uniforms.aShadows[0] = (float)bShadows;
    uniforms.aShadows[1] = (float)GraphicsSettings::On(GraphicsSettings::Switch::PHONG_SHADING);

    const auto aWorldLights = std::array<Vec3, 3>{ g_lightDirs[1].dir, g_lightDirs[2].dir, g_vLightDirExtra };

    for (size_t i = 0; i < aWorldLights.size(); i++)
    {
        uniforms.aWorldLightDirs[i][0] = aWorldLights[i].x;
        uniforms.aWorldLightDirs[i][1] = aWorldLights[i].y;
        uniforms.aWorldLightDirs[i][2] = aWorldLights[i].z;
    }

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

void HwRenderer::SubmitVertices(const std::vector<HwMesh::Vertex>& vertices, const Draw& draw)
{
    if (vertices.empty() || s_draws.size() >= MAX_DRAWS)
    {
        return;
    }

    auto dynamic = draw;

    dynamic.pBlock        = nullptr;
    dynamic.nDynamicFirst = (int)s_dynamic.size();
    dynamic.nDynamicCount = (int)vertices.size();

    s_dynamic.insert(s_dynamic.end(), vertices.begin(), vertices.end());
    s_draws.push_back(dynamic);
}

void HwRenderer::SetPointLight(float flX, float flY, float flZ, float flRadius)
{
    s_aPointLight[0] = flX;
    s_aPointLight[1] = flY;
    s_aPointLight[2] = flZ;
    s_aPointLight[3] = flRadius;
}

void HwRenderer::ForgetMeshes()
{
    for (const auto& [key, mesh] : s_meshes)
    {
        SDL_ReleaseGPUBuffer(s_pDevice, mesh.pBuffer);
    }

    s_meshes.clear();
}

unsigned int HwRenderer::Frame()
{
    return s_nFrame;
}

HwRenderer::Texture HwRenderer::Render(Renderer renderer, int nWidth, int nHeight, const unsigned char* pPalette)
{
    SDL_Renderer* const pRenderer = renderer.As<SDL_Renderer>();

    using GraphicsSettings::Switch;

    s_bTextures = GraphicsSettings::On(Switch::TEXTURES);
    s_bLinear   = GraphicsSettings::On(Switch::LINEAR_FILTERING);
    s_nFrame++;

    // The cached meshes carry their normals, so a change of shading builds them again.
    if (GraphicsSettings::On(Switch::SMOOTH_SHADING) != s_bSmooth)
    {
        s_bSmooth = GraphicsSettings::On(Switch::SMOOTH_SHADING);

        ForgetMeshes();
    }

    const auto clear = [&]
    {
        s_draws.clear();
        s_dynamic.clear();
        s_aPointLight[3] = 0.0f;
    };

    if (nWidth <= 0 || nHeight <= 0 || !Ready(pRenderer) || !Fit(pRenderer, nWidth, nHeight))
    {
        clear();

        return Texture{};
    }

    SDL_GPUCommandBuffer* const pCommands = SDL_AcquireGPUCommandBuffer(s_pDevice);

    if (pCommands == nullptr)
    {
        clear();

        return Texture{};
    }

    UploadMeshes(pCommands);

    const auto bShadows = GraphicsSettings::On(Switch::SHADOW_MAPS);

    auto frame = HwPipeline::FrameUniforms{};

    if (bShadows)
    {
        const auto aLightDirs = std::array<Vec3, 2>{ g_lightDirs[1].dir, g_lightDirs[2].dir };

        for (size_t i = 0; i < aLightDirs.size(); i++)
        {
            const auto lightClipFromWorld = LightClipFromWorld(aLightDirs[i]);

            std::memcpy(frame.aLightClipFromWorld[i], lightClipFromWorld.data(), sizeof(frame.aLightClipFromWorld[i]));

            DrawShadowMap(pCommands, s_apShadowMaps[i], lightClipFromWorld);
        }
    }

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

    const auto colours = PaletteUniforms(pPalette, bShadows);

    SDL_GPUSampler* const pSampler = [&]
    {
        if (s_bLinear)
        {
            return s_pLinear;
        }

        return s_pNearest;
    }();

    // The frame's own vertices are untextured, one range a draw.
    auto dynamicMesh = GPUMesh{ s_pDynamicBuffer, std::vector<GPURange>(1) };

    SDL_GPURenderPass* pPass  = nullptr;
    auto               bArena = false;

    // The draws come back to front, as the original painted them. Painted in order, each takes a
    // depth slice in front of the last, so it covers what came before and still hides its own faces.
    const auto bPainters = GraphicsSettings::On(Switch::PAINTERS_ORDER);
    const auto flSlice   = 1.0f / (float)(std::max)(s_draws.size(), (size_t)1);

    for (size_t nDraw = 0; nDraw < s_draws.size(); nDraw++)
    {
        const auto& draw = s_draws[nDraw];

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
            SDL_PushGPUVertexUniformData(pCommands, 1, &frame, sizeof(frame));
            SDL_PushGPUFragmentUniformData(pCommands, 1, &colours, sizeof(colours));
        }

        // A block draws from its cached mesh, and the frame's own vertices from the dynamic buffer.
        const auto* const pMesh = [&]() -> const GPUMesh*
        {
            if (draw.pBlock == nullptr)
            {
                dynamicMesh.ranges[0] = GPURange{ (Uint32)draw.nDynamicFirst, (Uint32)draw.nDynamicCount, nullptr };

                return &dynamicMesh;
            }

            const auto it = s_meshes.find(MeshKey{ draw.pBlock, draw.pMaterials });

            if (it == s_meshes.end())
            {
                return nullptr;
            }

            return &it->second;
        }();

        if (pMesh == nullptr || pMesh->pBuffer == nullptr || draw.nViewportWidth <= 0 || draw.nViewportHeight <= 0)
        {
            continue;
        }

        auto viewport = SDL_GPUViewport{ (float)draw.nViewportX, (float)draw.nViewportY, (float)draw.nViewportWidth, (float)draw.nViewportHeight, 0.0f, 1.0f };

        if (bPainters)
        {
            viewport.max_depth = 1.0f - (float)nDraw * flSlice;
            viewport.min_depth = viewport.max_depth - flSlice;
        }

        const auto binding  = SDL_GPUBufferBinding{ pMesh->pBuffer, 0 };
        const auto uniforms = ClipUniforms(draw);

        SDL_SetGPUViewport(pPass, &viewport);
        SDL_BindGPUVertexBuffers(pPass, 0, &binding, 1);
        SDL_PushGPUVertexUniformData(pCommands, 0, &uniforms, sizeof(uniforms));

        for (const auto& range : pMesh->ranges)
        {
            // The palette path never textures.
            const auto bTextured = s_bTextures && range.pTexture != nullptr;

            SDL_GPUTexture* const pTexture = [&]
            {
                if (bTextured)
                {
                    return range.pTexture;
                }

                return s_pWhite;
            }();

            const SDL_GPUTextureSamplerBinding aSampled[] = {
                { pTexture, pSampler },
                { s_apShadowMaps[0], s_pShadowSampler },
                { s_apShadowMaps[1], s_pShadowSampler },
            };

            const auto textureUniforms = HwPipeline::TextureUniforms{ (int)bTextured };

            SDL_BindGPUFragmentSamplers(pPass, 0, aSampled, SDL_arraysize(aSampled));
            SDL_PushGPUFragmentUniformData(pCommands, 0, &textureUniforms, sizeof(textureUniforms));
            SDL_DrawGPUPrimitives(pPass, range.nCount, 1, range.nFirst, 0);
        }
    }

    // A frame with nothing to draw still clears.
    if (pPass == nullptr)
    {
        pPass = SDL_BeginGPURenderPass(pCommands, &target, 1, &depth);
    }

    SDL_EndGPURenderPass(pPass);
    SDL_SubmitGPUCommandBuffer(pCommands);

    clear();

    return Texture{ s_pTexture };
}
