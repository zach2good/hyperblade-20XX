// Builds hardware triangle lists from small hand-made mesh blocks.

#include "graphics/hwmesh.h"
#include "graphics/meshblock.h"
#include "graphics/meshmaterial.h"
#include "graphics/texcoord.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

static int s_nFailed = 0;

static void Expect(bool bOK, const char* pszWhat)
{
    if (!bOK)
    {
        std::printf("failed: %s\n", pszWhat);
        s_nFailed++;
    }
}

struct FaceSpec
{
    std::vector<short> corners;
    std::vector<short> uvs; // Empty for a face without UVs.
    char               cMaterial;
    Vec3               vNormal;
    bool               bUVSkip;
};

template <class T>
static void Append(std::vector<unsigned char>& bytes, const T& value)
{
    const auto nAt = bytes.size();
    bytes.resize(nAt + sizeof(T));
    std::memcpy(&bytes[nAt], &value, sizeof(T));
}

template <class T>
static void Poke(std::vector<unsigned char>& bytes, std::size_t nAt, const T& value)
{
    std::memcpy(&bytes[nAt], &value, sizeof(T));
}

// Header, face offsets, vertices, normals, texture coordinates, then the faces, as the loader reads them.
static std::vector<unsigned char> LayOut(const std::vector<Vec3>& positions, short nNormalCount, const std::vector<TexCoord>& coords, const std::vector<FaceSpec>& faces)
{
    auto bytes = std::vector<unsigned char>(0x22 + 2 * faces.size());

    Poke(bytes, 0x06, static_cast<short>(faces.size()));
    Poke(bytes, 0x08, static_cast<short>(positions.size()));
    Poke(bytes, 0x0a, nNormalCount);
    Poke(bytes, 0x0c, static_cast<short>(coords.size()));
    Poke(bytes, 0x20, static_cast<short>(bytes.size()));

    for (const auto& vPosition : positions)
    {
        Append(bytes, vPosition);
    }

    for (auto nNormal = 0; nNormal < nNormalCount; nNormal++)
    {
        Append(bytes, Vec3{ -99.0f, -99.0f, -99.0f });
    }

    for (const auto& uv : coords)
    {
        Append(bytes, uv);
    }

    for (auto nFace = 0; nFace < static_cast<int>(faces.size()); nFace++)
    {
        const auto& face = faces[nFace];

        Poke(bytes, 0x22 + 2 * nFace, static_cast<short>(bytes.size()));

        const auto cFlags = [&]
        {
            auto cBits = 0;

            if (!face.uvs.empty())
            {
                cBits |= kFaceFlagHasUV;
            }

            if (face.bUVSkip)
            {
                cBits |= kFaceFlagUVSkip;
            }

            return static_cast<unsigned char>(cBits);
        }();

        Append(bytes, static_cast<short>(face.corners.size()));
        Append(bytes, cFlags);
        Append(bytes, face.cMaterial);
        Append(bytes, face.vNormal);

        for (const auto nCorner : face.corners)
        {
            Append(bytes, nCorner);
        }

        if (face.bUVSkip)
        {
            for (auto nCorner = 0; nCorner < static_cast<int>(face.corners.size()); nCorner++)
            {
                Append(bytes, static_cast<short>(77));
            }
        }

        if (!face.uvs.empty())
        {
            for (const auto nUV : face.uvs)
            {
                Append(bytes, nUV);
            }

            Append(bytes, static_cast<short>(-1));
        }
    }

    Poke(bytes, 0x00, static_cast<int>(bytes.size()));

    return bytes;
}

// Vertex k of the block sits at (k, 10k, 100k).
static bool AtVertex(const HwMesh::Vertex& vertex, int nVertex)
{
    return vertex.aPosition[0] == nVertex && vertex.aPosition[1] == 10.0f * nVertex && vertex.aPosition[2] == 100.0f * nVertex;
}

static bool IsTriangle(const HwMesh::Vertex* pVertices, int nA, int nB, int nC)
{
    return AtVertex(pVertices[0], nA) && AtVertex(pVertices[1], nB) && AtVertex(pVertices[2], nC);
}

// Coordinate k of the block is (k + 0.25, k + 0.75).
static bool HasUV(const HwMesh::Vertex& vertex, int nUV)
{
    return vertex.aUV[0] == nUV + 0.25f && vertex.aUV[1] == nUV + 0.75f;
}

static bool HasNormal(const HwMesh::Vertex& vertex, float flX, float flY, float flZ)
{
    return vertex.aNormal[0] == flX && vertex.aNormal[1] == flY && vertex.aNormal[2] == flZ;
}

static MeshMaterial MakeMaterial(int nColour, float flAmbient, float flDiffuse, float flEmissive, short nBackIndex)
{
    auto material       = MeshMaterial();
    material.nColour    = nColour;
    material.flAmbient  = flAmbient;
    material.flDiffuse  = flDiffuse;
    material.flEmissive = flEmissive;
    material.nBackIndex = nBackIndex;

    return material;
}

static bool IsRange(const HwMesh::Range& range, std::uint32_t nFirst, std::uint32_t nCount, const TexCacheEntry* pTexture)
{
    return range.nFirst == nFirst && range.nCount == nCount && range.pTexture == pTexture;
}

// Never dereferenced: Build only compares them.
static TexCacheEntry* FakeTexture(std::uintptr_t nAddress)
{
    return reinterpret_cast<TexCacheEntry*>(nAddress);
}

static void CheckRanges(const std::vector<Vec3>& positions, const std::vector<TexCoord>& coords)
{
    auto* const pTextureA = FakeTexture(0x1000);
    auto* const pTextureB = FakeTexture(0x2000);

    const auto vUp = Vec3{ 0.0f, 0.0f, 1.0f };

    const auto faces = std::vector<FaceSpec>{
        { { 0, 1, 2 }, { 0, 1, 2 }, 0, vUp, false },
        { { 1, 2, 3 }, { 1, 2, 3 }, 2, vUp, false },
        { { 2, 3, 4 }, { 2, 3, 4 }, 1, vUp, false },
        { { 3, 4, 5 }, { 3, 4, 5 }, 0, vUp, false },
        { { 0, 1, 2 }, {}, 0, vUp, false },
        { { 3, 4, 5 }, { 3, 4, 5 }, 3, vUp, false },
    };

    const auto bytes = LayOut(positions, 0, coords, faces);

    MeshMaterial aMaterials[] = {
        MakeMaterial(0x10, 0.0f, 0.0f, 0.0f, -1),
        MakeMaterial(0x20, 0.0f, 0.0f, 0.0f, -1),
        MakeMaterial(0x30, 0.0f, 0.0f, 0.0f, -1),
        MakeMaterial(0x40, 0.0f, 0.0f, 0.0f, 4),
        MakeMaterial(0x50, 0.0f, 0.0f, 0.0f, -1),
    };

    aMaterials[0].pTexture = pTextureA;
    aMaterials[1].pTexture = pTextureB;
    aMaterials[3].pTexture = pTextureA;
    aMaterials[4].pTexture = pTextureB;

    const auto& block = *reinterpret_cast<const MeshBlock*>(bytes.data());
    const auto  mesh  = HwMesh::Build(block, aMaterials, 5, false);

    Expect(mesh.vertices.size() == 3 * 7, "ranged triangle count");
    Expect(mesh.ranges.size() == 3, "range count");

    if (mesh.vertices.size() != 3 * 7 || mesh.ranges.size() != 3)
    {
        return;
    }

    // Groups appear in the order their first face does.
    Expect(IsRange(mesh.ranges[0], 0, 9, pTextureA), "first texture range");
    Expect(IsRange(mesh.ranges[1], 9, 6, nullptr), "untextured range");
    Expect(IsRange(mesh.ranges[2], 15, 6, pTextureB), "second texture range");

    const auto* const pTri = mesh.vertices.data();

    Expect(IsTriangle(pTri, 0, 2, 1) && IsTriangle(pTri + 3, 3, 5, 4) && IsTriangle(pTri + 6, 3, 5, 4), "first texture keeps face order");
    Expect(pTri[6].nColour == 0x40, "two-sided front in front texture");
    Expect(IsTriangle(pTri + 9, 1, 3, 2) && pTri[9].nColour == 0x30, "untextured material");
    Expect(IsTriangle(pTri + 12, 0, 2, 1) && pTri[12].nColour == 0x10, "textured material without UVs untextured");
    Expect(IsTriangle(pTri + 15, 2, 4, 3) && pTri[15].nColour == 0x20, "second texture face");
    Expect(IsTriangle(pTri + 18, 3, 4, 5) && pTri[18].nColour == 0x50 && HasNormal(pTri[18], 0.0f, 0.0f, -1.0f), "back copy in back texture");
}

static bool NearNormal(const HwMesh::Vertex& vertex, float flX, float flY, float flZ)
{
    const auto flDX = vertex.aNormal[0] - flX;
    const auto flDY = vertex.aNormal[1] - flY;
    const auto flDZ = vertex.aNormal[2] - flZ;

    return flDX * flDX + flDY * flDY + flDZ * flDZ < 1e-6f;
}

// Two faces at a shallow angle share their edge's normals, and a third at a right angle keeps its own.
static void CheckSmooth()
{
    const auto positions = std::vector<Vec3>{
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 0.5f },
        { 0.0f, 0.0f, 1.0f },
    };

    const auto faces = std::vector<FaceSpec>{
        { { 0, 1, 2 }, {}, 0, Vec3{ 0.0f, 0.0f, 1.0f }, false },
        { { 1, 3, 2 }, {}, 0, Vec3{ 0.0f, 0.6f, 0.8f }, false },
        { { 0, 4, 1 }, {}, 0, Vec3{ 1.0f, 0.0f, 0.0f }, false },
    };

    const auto bytes = LayOut(positions, 0, {}, faces);

    const MeshMaterial aMaterials[] = { MakeMaterial(0x10, 0.0f, 0.0f, 0.0f, -1) };

    const auto& block = *reinterpret_cast<const MeshBlock*>(bytes.data());
    const auto  flat  = HwMesh::Build(block, aMaterials, 1, false);
    const auto  mesh  = HwMesh::Build(block, aMaterials, 1, true);

    Expect(mesh.vertices.size() == 9 && flat.vertices.size() == 9, "smooth triangle count");

    if (mesh.vertices.size() != 9 || flat.vertices.size() != 9)
    {
        return;
    }

    Expect(HasNormal(flat.vertices[1], 0.0f, 0.0f, 1.0f), "flat keeps the face normal");
    Expect(NearNormal(mesh.vertices[0], 0.0f, 0.0f, 1.0f), "a crease is not smoothed across");
    Expect(NearNormal(mesh.vertices[1], 0.0f, 0.31622777f, 0.9486833f), "a shallow edge is smoothed");
    Expect(NearNormal(mesh.vertices[6], 1.0f, 0.0f, 0.0f), "the creased face keeps its own");
}

int main()
{
    auto positions = std::vector<Vec3>();
    auto coords    = std::vector<TexCoord>();

    for (auto nIndex = 0; nIndex < 6; nIndex++)
    {
        positions.push_back(Vec3{ static_cast<float>(nIndex), 10.0f * nIndex, 100.0f * nIndex });
        coords.push_back(TexCoord{ nIndex + 0.25f, nIndex + 0.75f });
    }

    const auto vUp = Vec3{ 0.0f, 0.0f, 1.0f };

    const auto faces = std::vector<FaceSpec>{
        { { 0, 1, 2 }, {}, 0, vUp, false },
        { { 0, 1, 2, 3 }, {}, 0, vUp, false },
        { { 0, 1, 2, 3, 4 }, { 4, 3, 2, 1, 0 }, 0, vUp, true },
        { { 1, 2, 3 }, { 5, 1, 2 }, 0, vUp, false },
        { { 3, 4, 5 }, {}, 1, vUp, false },
        { { 0, 1, 99 }, {}, 0, vUp, false },
        { { 0, 1, 2 }, {}, 7, vUp, false },
        { { 0, 1, 2 }, { 0, 1, 6 }, 0, vUp, false },
        { { 0, 1, 2 }, {}, 3, vUp, false },
    };

    const auto bytes = LayOut(positions, 2, coords, faces);

    const MeshMaterial aMaterials[] = {
        MakeMaterial(0x7f102030, 0.25f, 0.5f, 0.125f, -1),
        MakeMaterial(0x7f405060, 0.5f, 0.75f, 0.0f, 2),
        MakeMaterial(0x7f708090, 0.0625f, 0.375f, 0.25f, -1),
        MakeMaterial(0x7fa0b0c0, 0.0f, 0.0f, 0.0f, 9),
    };

    const auto& block    = *reinterpret_cast<const MeshBlock*>(bytes.data());
    const auto  mesh     = HwMesh::Build(block, aMaterials, 4, false);
    const auto& vertices = mesh.vertices;

    Expect(mesh.ranges.size() == 1 && IsRange(mesh.ranges[0], 0, static_cast<std::uint32_t>(vertices.size()), nullptr), "untextured block is one range");

    CheckRanges(positions, coords);

    // One, two, three, one, two front and one back, then the bad back index face's front alone.
    Expect(vertices.size() == 3 * 10, "triangle count");

    if (vertices.size() != 3 * 10)
    {
        std::printf("%d failed\n", s_nFailed);

        return 1;
    }

    const auto* pTri = vertices.data();

    Expect(IsTriangle(pTri, 0, 2, 1), "triangle fanned backwards");
    Expect(pTri[0].aUV[0] == 0.0f && pTri[0].aUV[1] == 0.0f, "no UVs reads as zero");
    Expect(pTri[0].nColour == 0x7f102030, "front colour");
    Expect(pTri[0].flDiffuse == 0.5f, "front diffuse");
    Expect(pTri[0].flAmbient == 0.375f, "ambient carries emissive");
    Expect(HasNormal(pTri[1], 0.0f, 0.0f, 1.0f), "face normal");
    pTri += 3;

    Expect(IsTriangle(pTri, 0, 3, 2), "quad first triangle");
    Expect(IsTriangle(pTri + 3, 0, 2, 1), "quad second triangle");
    pTri += 6;

    Expect(IsTriangle(pTri, 0, 4, 3), "pentagon first triangle");
    Expect(IsTriangle(pTri + 3, 0, 3, 2), "pentagon second triangle");
    Expect(IsTriangle(pTri + 6, 0, 2, 1), "pentagon third triangle");
    Expect(HasUV(pTri[0], 4) && HasUV(pTri[1], 0) && HasUV(pTri[2], 1), "UVs past the second corner block");
    pTri += 9;

    Expect(IsTriangle(pTri, 1, 3, 2), "textured triangle");
    Expect(HasUV(pTri[0], 5) && HasUV(pTri[1], 2) && HasUV(pTri[2], 1), "UVs follow their corners");
    pTri += 3;

    Expect(IsTriangle(pTri, 3, 5, 4), "two-sided front");
    Expect(pTri[0].nColour == 0x7f405060 && pTri[0].flAmbient == 0.5f, "two-sided front material");
    Expect(HasNormal(pTri[2], 0.0f, 0.0f, 1.0f), "two-sided front normal");
    pTri += 3;

    Expect(IsTriangle(pTri, 3, 4, 5), "back copy reversed");
    Expect(pTri[0].nColour == 0x7f708090 && pTri[1].flDiffuse == 0.375f && pTri[2].flAmbient == 0.3125f, "back copy lit with back material");
    Expect(HasNormal(pTri[0], 0.0f, 0.0f, -1.0f), "back copy normal negated");
    pTri += 3;

    Expect(IsTriangle(pTri, 0, 2, 1) && pTri[0].nColour == 0x7fa0b0c0, "back index out of range keeps the front");

    CheckSmooth();

    std::printf("%d failed\n", s_nFailed);

    if (s_nFailed != 0)
    {
        return 1;
    }

    return 0;
}
