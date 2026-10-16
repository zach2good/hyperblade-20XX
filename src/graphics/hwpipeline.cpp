#include "graphics/hwpipeline.h"
#include "graphics/hwmesh.h"
#include "mesh.frag.dxil.h"
#include "mesh.frag.spv.h"
#include "mesh.vert.dxil.h"
#include "mesh.vert.spv.h"
#include <SDL3/SDL.h>
#include <cstddef>

namespace
{

SDL_GPUShader* CreateShader(SDL_GPUDevice* pDevice, SDL_GPUShaderStage stage, const unsigned char* pDXIL, std::size_t nDXILSize, const unsigned char* pSPIRV, std::size_t nSPIRVSize, Uint32 nSamplers, Uint32 nUniformBuffers)
{
    auto info                = SDL_GPUShaderCreateInfo{};
    info.entrypoint          = "main";
    info.stage               = stage;
    info.num_samplers        = nSamplers;
    info.num_uniform_buffers = nUniformBuffers;

    if ((SDL_GetGPUShaderFormats(pDevice) & SDL_GPU_SHADERFORMAT_DXIL) != 0)
    {
        info.format    = SDL_GPU_SHADERFORMAT_DXIL;
        info.code      = pDXIL;
        info.code_size = nDXILSize;
    }
    else
    {
        info.format    = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code      = pSPIRV;
        info.code_size = nSPIRVSize;
    }

    return SDL_CreateGPUShader(pDevice, &info);
}

} // namespace

unsigned int HwPipeline::ColourFormat()
{
    return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
}

unsigned int HwPipeline::DepthFormat(Device device)
{
    SDL_GPUDevice* const pDevice = device.As<SDL_GPUDevice>();

    for (const auto format : { SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D24_UNORM })
    {
        if (SDL_GPUTextureSupportsFormat(pDevice, format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
        {
            return format;
        }
    }

    return SDL_GPU_TEXTUREFORMAT_D16_UNORM;
}

HwPipeline::Pipeline HwPipeline::CreateMeshPipeline(Device device, bool bFrontClockwise)
{
    SDL_GPUDevice* const pDevice = device.As<SDL_GPUDevice>();

    SDL_GPUShader* const pVertex   = CreateShader(pDevice, SDL_GPU_SHADERSTAGE_VERTEX, mesh_vert_dxil, sizeof(mesh_vert_dxil), mesh_vert_spv, sizeof(mesh_vert_spv), 0, 2);
    SDL_GPUShader* const pFragment = CreateShader(pDevice, SDL_GPU_SHADERSTAGE_FRAGMENT, mesh_frag_dxil, sizeof(mesh_frag_dxil), mesh_frag_spv, sizeof(mesh_frag_spv), 1, 1);

    SDL_GPUGraphicsPipeline* pPipeline = nullptr;

    if (pVertex != nullptr && pFragment != nullptr)
    {
        auto buffer  = SDL_GPUVertexBufferDescription{};
        buffer.pitch = sizeof(HwMesh::Vertex);

        const SDL_GPUVertexAttribute aAttributes[] = {
            { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(HwMesh::Vertex, aPosition) },
            { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(HwMesh::Vertex, aNormal) },
            { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(HwMesh::Vertex, aUV) },
            { 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4, offsetof(HwMesh::Vertex, nColour) },
            { 4, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(HwMesh::Vertex, flDiffuse) },
            { 5, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(HwMesh::Vertex, flAmbient) },
        };

        auto colour   = SDL_GPUColorTargetDescription{};
        colour.format = (SDL_GPUTextureFormat)ColourFormat();

        auto info                                          = SDL_GPUGraphicsPipelineCreateInfo{};
        info.vertex_shader                                 = pVertex;
        info.fragment_shader                               = pFragment;
        info.vertex_input_state.vertex_buffer_descriptions = &buffer;
        info.vertex_input_state.num_vertex_buffers         = 1;
        info.vertex_input_state.vertex_attributes          = aAttributes;
        info.vertex_input_state.num_vertex_attributes      = SDL_arraysize(aAttributes);
        info.primitive_type                                = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode                    = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode                    = SDL_GPU_CULLMODE_BACK;
        info.rasterizer_state.front_face                   = [&]
        {
            if (bFrontClockwise)
            {
                return SDL_GPU_FRONTFACE_CLOCKWISE;
            }

            return SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        }();
        info.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        info.depth_stencil_state.enable_depth_test  = true;
        info.depth_stencil_state.enable_depth_write = true;
        info.target_info.color_target_descriptions  = &colour;
        info.target_info.num_color_targets          = 1;
        info.target_info.depth_stencil_format       = (SDL_GPUTextureFormat)DepthFormat(device);
        info.target_info.has_depth_stencil_target   = true;

        pPipeline = SDL_CreateGPUGraphicsPipeline(pDevice, &info);
    }

    if (pVertex != nullptr)
    {
        SDL_ReleaseGPUShader(pDevice, pVertex);
    }

    if (pFragment != nullptr)
    {
        SDL_ReleaseGPUShader(pDevice, pFragment);
    }

    return Pipeline{ pPipeline };
}

void HwPipeline::Release(Device device, Pipeline pipeline)
{
    if (pipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(device.As<SDL_GPUDevice>(), pipeline.As<SDL_GPUGraphicsPipeline>());
    }
}

HwPipeline::Sampler HwPipeline::CreateSampler(Device device, bool bLinear)
{
    const auto filter = [&]
    {
        if (bLinear)
        {
            return SDL_GPU_FILTER_LINEAR;
        }

        return SDL_GPU_FILTER_NEAREST;
    }();

    const auto mipmapMode = [&]
    {
        if (bLinear)
        {
            return SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        }

        return SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    }();

    auto info           = SDL_GPUSamplerCreateInfo{};
    info.min_filter     = filter;
    info.mag_filter     = filter;
    info.mipmap_mode    = mipmapMode;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    info.max_lod        = 1000.0f;

    return Sampler{ SDL_CreateGPUSampler(device.As<SDL_GPUDevice>(), &info) };
}

void HwPipeline::Release(Device device, Sampler sampler)
{
    if (sampler)
    {
        SDL_ReleaseGPUSampler(device.As<SDL_GPUDevice>(), sampler.As<SDL_GPUSampler>());
    }
}
