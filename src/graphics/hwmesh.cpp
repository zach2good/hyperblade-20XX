#include "graphics/hwmesh.h"
#include "graphics/meshblock.h"
#include "graphics/meshface.h"
#include "graphics/meshmaterial.h"
#include "graphics/texcoord.h"
#include <cmath>
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

// Faces meeting at more than this angle keep a hard edge between them.
constexpr auto CREASE_COS = 0.5f;

using FacesByVertex = std::vector<std::vector<const MeshFace*>>;

FacesByVertex BuildFacesByVertex(const MeshBlock& block, int nMaterialCount)
{
    auto faces = FacesByVertex((size_t)block.nVertexCount);

    for (auto nFace = 0; nFace < block.nFaceCount; nFace++)
    {
        auto& face = *block.Face(nFace);

        if (!FaceInRange(block, face, nMaterialCount))
        {
            continue;
        }

        for (auto nCorner = 0; nCorner < face.nCorners; nCorner++)
        {
            faces[(size_t)face.aCorners[nCorner]].push_back(&face);
        }
    }

    return faces;
}

Vec3 CornerNormal(const FacesByVertex& faces, const MeshFace& face, int nCorner)
{
    auto vSum = Vec3{ 0.0f, 0.0f, 0.0f };

    for (const MeshFace* const pOther : faces[(size_t)face.aCorners[nCorner]])
    {
        const auto& vOther = pOther->vNormal;

        if (vOther.x * face.vNormal.x + vOther.y * face.vNormal.y + vOther.z * face.vNormal.z >= CREASE_COS)
        {
            vSum = Vec3{ vSum.x + vOther.x, vSum.y + vOther.y, vSum.z + vOther.z };
        }
    }

    const auto flLength = std::sqrt(vSum.x * vSum.x + vSum.y * vSum.y + vSum.z * vSum.z);

    if (flLength <= 0.0f)
    {
        return face.vNormal;
    }

    return Vec3{ vSum.x / flLength, vSum.y / flLength, vSum.z / flLength };
}

void AppendTriangle(std::vector<HwMesh::Vertex>& vertices, const MeshBlock& block, MeshFace& face, const MeshMaterial& material, const std::vector<Vec3>& normals, int nA, int nB, int nC)
{
    for (const auto nCorner : { nA, nB, nC })
    {
        const auto& vPosition = block.Vertices()[face.aCorners[nCorner]];
        const auto& vNormal   = normals[(size_t)nCorner];

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

HwMesh::Mesh HwMesh::Build(const MeshBlock& block, const MeshMaterial* pMaterials, int nMaterialCount, bool bSmooth)
{
    auto faces = FacesByVertex();

    if (bSmooth)
    {
        faces = BuildFacesByVertex(block, nMaterialCount);
    }

    auto normals = std::vector<Vec3>();

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

        normals.assign((size_t)face.nCorners, face.vNormal);

        if (bSmooth)
        {
            for (auto nCorner = 0; nCorner < face.nCorners; nCorner++)
            {
                normals[(size_t)nCorner] = CornerNormal(faces, face, nCorner);
            }
        }

        // D3DRend::DrawFace fans from corner zero back through the last corner to the first.
        for (auto nCorner = face.nCorners - 1; nCorner >= 2; nCorner--)
        {
            AppendTriangle(frontList, block, face, front, normals, 0, nCorner, nCorner - 1);
        }

        // The original reads past the table for a back index out of range.
        if (front.nBackIndex < 0 || front.nBackIndex >= nMaterialCount)
        {
            continue;
        }

        const auto& back     = pMaterials[front.nBackIndex];
        auto&       backList = group(face, back);

        for (auto& vNormal : normals)
        {
            vNormal = Vec3{ -vNormal.x, -vNormal.y, -vNormal.z };
        }

        for (auto nCorner = 1; nCorner < face.nCorners - 1; nCorner++)
        {
            AppendTriangle(backList, block, face, back, normals, 0, nCorner, nCorner + 1);
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
