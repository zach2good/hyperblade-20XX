// The hardware renderer's GPU pipelines and the uniform blocks its shaders read.

#ifndef HYPERBLADE_HWPIPELINE_H
#define HYPERBLADE_HWPIPELINE_H

#include "common/handle.h"

namespace HwPipeline
{

using Device   = Handle<struct HwPipelineDeviceTag>;
using Pipeline = Handle<struct HwPipelineTag>;

// Vertex uniform slot 0 of the mesh pipeline.
struct MeshUniforms
{
    // Row-major, taking column vectors: clip = M * (x, y, z, 1).
    float aClipFromModel[16];

    // The light the face is lit by, in the model's space, w unused.
    float aLightDir[4];
};

static_assert(sizeof(MeshUniforms) == 80);

// The colour format every hardware target uses, and the depth format this device supports.
unsigned int ColourFormat();
unsigned int DepthFormat(Device device);

// Draws HwMesh::Vertex triangle lists, depth tested, culling back faces. Front faces wind
// clockwise in clip space when bFrontClockwise is set.
Pipeline CreateMeshPipeline(Device device, bool bFrontClockwise);
void     Release(Device device, Pipeline pipeline);

} // namespace HwPipeline

#endif
