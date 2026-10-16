#include "graphics/hwmesh.h"
#include "graphics/meshblock.h"
#include "graphics/meshface.h"
#include "graphics/meshmaterial.h"
#include "graphics/texcoord.h"
#include <utility>

namespace
{

// The original trusts every index in the file; a face with one out of range is skipped instead.
bool FaceInRange(const MeshBlock& block, MeshFace& face, int nMaterialCount)
{
    if (face.nCorners < 3 || face.cMaterial < 0 || face.cMaterial >= nMaterialCount)
    {
        return false;
    }

    const auto bHasUV = (face.cFlags & FACE_FLAG_HAS_UV) != 0;

    for (auto nCorner = 0; nCorner < face.nCorners; nCorner++)
    {
        const auto nVertex = face.aCorners[nCorner];

        if (nVertex < 0 || nVertex >= block.nVertexCount)
        {
            return false;
        }

        if (bHasUV)
        {
            const auto nUV = face.UVIndices()[nCorner];

            if (nUV < 0 || nUV >= block.nCoordBytes)
            {
                return false;
            }
        }
    }

    return true;
}

void AppendTriangle(std::vector<HwMesh::Vertex>& vertices, const MeshBlock& block, MeshFace& face, const MeshMaterial& material, const Vec3& vNormal, int nA, int nB, int nC)
{
    for (const auto nCorner : { nA, nB, nC })
    {
        const auto& vPosition = block.Vertices()[face.aCorners[nCorner]];

        const auto uv = [&]
        {
            if ((face.cFlags & FACE_FLAG_HAS_UV) != 0)
            {
                return block.Coords()[face.UVIndices()[nCorner]];
            }

            return TexCoord{ 0.0f, 0.0f };
        }();

        vertices.push_back(HwMesh::Vertex{
            { vPosition.x, vPosition.y, vPosition.z },
            { vNormal.x, vNormal.y, vNormal.z },
            { uv.tu, uv.tv },
            static_cast<std::uint32_t>(material.nColour),
            material.flDiffuse,
            material.flAmbient + material.flEmissive,
        });
    }
}

void AppendShadowTriangle(std::vector<HwMesh::Vertex>& vertices, const Vec3* pPoints, const MeshFace& face, const MeshMaterial& material, const Vec3& vNormal, int nA, int nB, int nC)
{
    for (const auto nCorner : { nA, nB, nC })
    {
        const auto& vPosition = pPoints[face.aCorners[nCorner]];

        vertices.push_back(HwMesh::Vertex{
            { vPosition.x, vPosition.y, vPosition.z },
            { vNormal.x, vNormal.y, vNormal.z },
            { 0.0f, 0.0f },
            static_cast<std::uint32_t>(material.nColour),
            material.flDiffuse,
            material.flAmbient + material.flEmissive,
        });
    }
}

} // namespace

std::vector<HwMesh::Vertex> HwMesh::BuildShadow(const Vec3* pPoints, int nPointCount, const ShadowFace* pFaces, int nFaceCount, const MeshMaterial* pMaterials, int nMaterialCount, const Vec3& vNormal)
{
    auto vertices = std::vector<Vertex>();

    for (auto nFace = 0; nFace < nFaceCount; nFace++)
    {
        const auto& face = pFaces[nFace].face;

        // A shadow face runs its corners on to four.
        if (face.nCorners < 3 || face.nCorners > 4 || face.cMaterial < 0 || face.cMaterial >= nMaterialCount)
        {
            continue;
        }

        auto bInRange = true;

        for (auto nCorner = 0; nCorner < face.nCorners; nCorner++)
        {
            bInRange = bInRange && face.aCorners[nCorner] >= 0 && face.aCorners[nCorner] < nPointCount;
        }

        if (!bInRange)
        {
            continue;
        }

        const auto& front = pMaterials[face.cMaterial];

        for (auto nCorner = face.nCorners - 1; nCorner >= 2; nCorner--)
        {
            AppendShadowTriangle(vertices, pPoints, face, front, vNormal, 0, nCorner, nCorner - 1);
        }

        // The original reads before the table when there is no back material; the front stands in.
        const auto& back = [&]() -> const MeshMaterial&
        {
            if (front.nBackIndex < 0 || front.nBackIndex >= nMaterialCount)
            {
                return front;
            }

            return pMaterials[front.nBackIndex];
        }();

        const auto vFlipped = Vec3{ -vNormal.x, -vNormal.y, -vNormal.z };

        for (auto nCorner = 1; nCorner < face.nCorners - 1; nCorner++)
        {
            AppendShadowTriangle(vertices, pPoints, face, back, vFlipped, 0, nCorner, nCorner + 1);
        }
    }

    return vertices;
}

HwMesh::Mesh HwMesh::Build(const MeshBlock& block, const MeshMaterial* pMaterials, int nMaterialCount)
{
    // A block uses a handful of textures, so a linear search finds each group.
    auto groups = std::vector<std::pair<const TexCacheEntry*, std::vector<Vertex> > >();

    const auto group = [&](const MeshFace& face, const MeshMaterial& material) -> std::vector<Vertex>&
    {
        // D3DRend::DrawFace textures only a face that has UVs.
        const auto* const pTexture = [&]() -> const TexCacheEntry*
        {
            if ((face.cFlags & FACE_FLAG_HAS_UV) == 0)
            {
                return nullptr;
            }

            return material.pTexture;
        }();

        for (auto& [pGroupTexture, vertices] : groups)
        {
            if (pGroupTexture == pTexture)
            {
                return vertices;
            }
        }

        return groups.emplace_back(pTexture, std::vector<Vertex>()).second;
    };

    for (auto nFace = 0; nFace < block.nFaceCount; nFace++)
    {
        auto& face = *block.Face(nFace);

        if (!FaceInRange(block, face, nMaterialCount))
        {
            continue;
        }

        const auto& front     = pMaterials[face.cMaterial];
        auto&       frontList = group(face, front);

        // D3DRend::DrawFace fans from corner zero back through the last corner to the first.
        for (auto nCorner = face.nCorners - 1; nCorner >= 2; nCorner--)
        {
            AppendTriangle(frontList, block, face, front, face.vNormal, 0, nCorner, nCorner - 1);
        }

        // The original reads past the table for a back index out of range.
        if (front.nBackIndex < 0 || front.nBackIndex >= nMaterialCount)
        {
            continue;
        }

        const auto& back     = pMaterials[front.nBackIndex];
        const auto  vFlipped = Vec3{ -face.vNormal.x, -face.vNormal.y, -face.vNormal.z };
        auto&       backList = group(face, back);

        for (auto nCorner = 1; nCorner < face.nCorners - 1; nCorner++)
        {
            AppendTriangle(backList, block, face, back, vFlipped, 0, nCorner, nCorner + 1);
        }
    }

    auto mesh = Mesh{};

    for (const auto& [pTexture, vertices] : groups)
    {
        mesh.ranges.push_back(Range{ static_cast<std::uint32_t>(mesh.vertices.size()), static_cast<std::uint32_t>(vertices.size()), pTexture });
        mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());
    }

    return mesh;
}
