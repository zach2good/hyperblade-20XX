// Mesh blocks as the hardware renderer draws them: every face fanned into triangles, each corner
// carrying the face's normal and material, so the shader lights each face flat as the original
// did, with no index buffer.

#ifndef HYPERBLADE_HWMESH_H
#define HYPERBLADE_HWMESH_H

#include <cstdint>
#include <vector>

struct MeshBlock;
struct MeshMaterial;

namespace HwMesh
{

struct Vertex
{
    float         aPosition[3]; // In the bone's space, as the file stores it.
    float         aNormal[3];   // The face's.
    float         aUV[2];       // As the file stores them, before any software rescale.
    std::uint32_t nColour;      // The material's 0xAARRGGBB.
    float         flDiffuse;
    float         flAmbient; // Ambient plus emissive.
};

static_assert(sizeof(Vertex) == 44);

// Every face of the block as a triangle list. A face whose material has a back material is also
// emitted reversed, with the back material and the normal negated.
// Front triangles run clockwise in pixels with y down: the file's corners, which run anticlockwise.
std::vector<Vertex> Build(const MeshBlock& block, const MeshMaterial* pMaterials, int nMaterialCount);

} // namespace HwMesh

#endif
