// The hardware renderer's GPU pipelines and the uniform blocks its shaders read.

#ifndef HYPERBLADE_HWPIPELINE_H
#define HYPERBLADE_HWPIPELINE_H

#include "common/handle.h"

namespace HwPipeline
{

using Device   = Handle<struct HwPipelineDeviceTag>;
using Pipeline = Handle<struct HwPipelineTag>;
using Sampler  = Handle<struct HwSamplerTag>;

// Vertex uniform slot 0 of the mesh pipeline.
struct MeshUniforms
{
    // Row-major, taking column vectors: clip = M * (x, y, z, 1).
    float aClipFromModel[16];

    // The light the face is lit by, in the model's space; w is one in the drome pass, whose
    // palette ramps are walked on a curve.
    float aLightDir[4];

    // x is one when the material's low byte is a palette index, lit along the ramps as
    // ColorPalette::LookupColorIntensity does, and zero when the material holds RGB.
    float aFlags[4];
};

static_assert(sizeof(MeshUniforms) == 96);

// Vertex uniform slot 1, once a frame: how a material's colour becomes the face's lit colour.
struct ColourUniforms
{
    // Indices inside the large ramp step along it; every other index steps along the small one.
    int nLargeRampFirst;
    int nLargeRampLast;
    int nSmallRampLength;
    int nLargeRampLength;

    // The live palette, RGB from 0 to 1.
    float aPalette[256][4];
};

static_assert(sizeof(ColourUniforms) == 16 + 256 * 16);

// Fragment uniform slot 0, per draw.
struct TextureUniforms
{
    // Zero: the face's lit colour. One: its grey light level times the texel, as the original's
    // MODULATE blend drew.
    int nTextured;
    int anPadding[3];
};

static_assert(sizeof(TextureUniforms) == 16);

// The colour format every hardware target uses, and the depth format this device supports.
unsigned int ColourFormat();
unsigned int DepthFormat(Device device);

// Draws HwMesh::Vertex triangle lists, depth tested, culling back faces. Front faces wind
// clockwise in clip space when bFrontClockwise is set. Fragment sampler slot 0 takes the texture.
Pipeline CreateMeshPipeline(Device device, bool bFrontClockwise);
void     Release(Device device, Pipeline pipeline);

// Wraps on U and V, as UVs run past one; nearest or linear within and between mip levels.
Sampler CreateSampler(Device device, bool bLinear);
void    Release(Device device, Sampler sampler);

} // namespace HwPipeline

#endif
