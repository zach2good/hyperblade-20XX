// The hardware renderer's GPU pipelines and the uniform blocks its shaders read.

#ifndef HYPERBLADE_HWPIPELINE_H
#define HYPERBLADE_HWPIPELINE_H

#include "common/handle.h"

namespace HwPipeline
{

using Device   = Handle<struct HwPipelineDeviceTag>;
using Pipeline = Handle<struct HwPipelineTag>;
using Sampler  = Handle<struct HwSamplerTag>;

// Vertex uniform slot 0 of the mesh pipeline, per draw.
struct MeshUniforms
{
    // Row-major, taking column vectors: clip = M * (x, y, z, 1).
    float aClipFromModel[16];
    float aWorldFromModel[16];

    // The scene's three lights, at azimuths 303, 54 and 180, in the model's space; w is each
    // light's strength, zero for off.
    float aLightDirs[3][4];

    // x is one when the material's low byte is a palette index, lit along the ramps as
    // ColorPalette::LookupColorIntensity does, and zero when the material holds RGB. y is the
    // key light, the one the original lights the face by and the only one that can darken it,
    // or -1 for none. z is one in the drome pass, whose ramps are walked on a curve.
    float aFlags[4];

    // The camera's eye in the world; w is zero when the projection has no eye to find.
    float aEye[4];
};

static_assert(sizeof(MeshUniforms) == 208);

// Vertex uniform slot 1, once a frame: the clip spaces of the shadow maps of lights 303 and 54.
struct FrameUniforms
{
    float aLightClipFromWorld[2][16];
};

static_assert(sizeof(FrameUniforms) == 128);

// Fragment uniform slot 0, per draw.
struct TextureUniforms
{
    // Zero: the face's lit colour. One: its grey light level times the texel, as the original's
    // MODULATE blend drew.
    int nTextured;
    int anPadding[3];
};

static_assert(sizeof(TextureUniforms) == 16);

// Fragment uniform slot 1, once a frame: how a face's light level becomes its colour.
struct ColourUniforms
{
    // Indices inside the large ramp step along it; every other index steps along the small one.
    int nLargeRampFirst;
    int nLargeRampLast;
    int nSmallRampLength;
    int nLargeRampLength;

    // The rok's light in the world; w is its reach, zero for none.
    float aPointLight[4];

    // x is one when the shadow maps are drawn and sampled; y is one for Phong shading; z is how far
    // a shadow's edge spreads, in world units.
    float aShadows[4];

    // The scene's lights in the world, for Phong shading.
    float aWorldLightDirs[3][4];

    // The shadow maps' views again, and the semi-axes of the collision ellipsoid the dromers
    // stand on, so the arena can read its shadows off the ellipsoid.
    float aLightClipFromWorld[2][16];
    float aArenaEllipsoid[4];

    // The live palette, RGB from 0 to 1.
    float aPalette[256][4];
};

static_assert(sizeof(ColourUniforms) == 96 + 128 + 16 + 256 * 16);

// Vertex uniform slot 0 of the shadow pipeline, per draw.
struct ShadowUniforms
{
    float aLightClipFromModel[16];
};

static_assert(sizeof(ShadowUniforms) == 64);

// The colour format every hardware target uses, and the depth format this device supports.
unsigned int ColourFormat();
unsigned int DepthFormat(Device device);

// A depth format the device can both draw into and sample with a comparison.
unsigned int ShadowFormat(Device device);

// Draws HwMesh::Vertex triangle lists, depth tested, culling back faces. Front faces wind
// clockwise in clip space when bFrontClockwise is set. Fragment sampler slot 0 takes the texture,
// and slots 1 and 2 the shadow maps.
Pipeline CreateMeshPipeline(Device device, bool bFrontClockwise);

// Draws HwMesh::Vertex positions into a ShadowFormat target, both sides, biased away from the light.
Pipeline CreateShadowPipeline(Device device);
void     Release(Device device, Pipeline pipeline);

// Wraps on U and V, as UVs run past one; nearest or linear within and between mip levels.
Sampler CreateSampler(Device device, bool bLinear);

// Compares against a shadow map, filtered between its texels, clamped at its edges.
Sampler CreateShadowSampler(Device device);
void    Release(Device device, Sampler sampler);

} // namespace HwPipeline

#endif
