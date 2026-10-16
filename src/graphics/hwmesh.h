// Mesh blocks as the hardware renderer draws them: every face fanned into triangles, each corner
// carrying its normal and the face's material, with no index buffer. Flat, every corner takes
// the face's normal and the shader lights each face as the original did.

#ifndef HYPERBLADE_HWMESH_H
#define HYPERBLADE_HWMESH_H

#include <cstdint>
#include <vector>

class TexCacheEntry;
struct MeshBlock;
struct MeshMaterial;
struct ShadowFace;
struct Vec3;

namespace HwMesh
{

struct Vertex
{
    float         aPosition[3]; // In the bone's space, as the file stores it.
    float         aNormal[3];   // The face's, or the corner's own when smooth.
    float         aUV[2];       // As the file stores them, before any software rescale.
    std::uint32_t nColour;      // The material's 0xAARRGGBB.
    float         flDiffuse;
    float         flAmbient; // Ambient plus emissive.
};

static_assert(sizeof(Vertex) == 44);

// The triangles drawn with one texture, or with none when pTexture is null.
struct Range
{
    std::uint32_t        nFirst;
    std::uint32_t        nCount;
    const TexCacheEntry* pTexture;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<Range>  ranges;
};

// Every face of the block as a triangle list. A face whose material has a back material is also
// emitted reversed, with the back material and the normal negated.
// Front triangles run clockwise in pixels with y down: the file's corners, which run anticlockwise.
// Triangles are grouped by the texture of the material each was emitted with, in face order
// within a group. A face is textured only when its material has a texture and it has UVs.
// Smooth, a corner takes the mean normal of the faces at its vertex that bend less than the
// crease from its own, so hard edges stay hard.
Mesh Build(const MeshBlock& block, const MeshMaterial* pMaterials, int nMaterialCount, bool bSmooth);

// A shadow, from points the engine has already posed and laid on the arena. EngineObject's
// DrawTransformedMesh draws each face whichever way it faces, with the back material and the
// normal negated when flipped, so both windings are emitted.
std::vector<Vertex> BuildShadow(const Vec3* pPoints, int nPointCount, const ShadowFace* pFaces, int nFaceCount, const MeshMaterial* pMaterials, int nMaterialCount, const Vec3& vNormal);

} // namespace HwMesh

#endif
