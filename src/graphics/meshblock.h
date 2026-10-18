// The face offset table starts at 0x22. Packed to one: the floats at 0x0e and 0x1a are unaligned.

#ifndef HYPERBLADE_MESHBLOCK_H
#define HYPERBLADE_MESHBLOCK_H

#include "graphics/meshface.h"
#include "graphics/recordoffset.h"
#include "math/vector.h"

#pragma pack(push, 1)

struct MeshBlock
{
    // The record's size, which ModelComponent::ReadAll fills in after the read.
    int                           nSize;           // 0x00
    short                         nSlot;           // 0x04
    short                         nFaceCount;      // 0x06
    short                         nVertexCount;    // 0x08
    short                         nNormalCount;    // 0x0a
    short                         nCoordBytes;     // 0x0c
    Vec3                          vOrigin;         // 0x0e
    float                         flDepthBias;     // 0x1a
    unsigned short                wFlags;          // 0x1e
    RecordOffset<Vec3>            vertexOffset;    // 0x20
    RecordOffset<struct MeshFace> aFaceOffsets[1]; // 0x22, one per face

    // Bit five asks the renderer for the depth sort.
    unsigned char SortsFaces() const
    {
        return static_cast<unsigned char>(wFlags >> 5) & 1;
    }

    Vec3*            Vertices() const;
    struct MeshFace* Face(int nFace) const;
    struct TexCoord* Coords() const;
};

#pragma pack(pop)

inline Vec3* MeshBlock::Vertices() const
{
    return vertexOffset.In(this);
}

// The texture coordinates follow the vertices and their normals.
inline TexCoord* MeshBlock::Coords() const
{
    return reinterpret_cast<TexCoord*>(Vertices() + nVertexCount + nNormalCount);
}

inline MeshFace* MeshBlock::Face(int nFace) const
{
    return aFaceOffsets[nFace].In(this);
}

static_assert(sizeof(MeshBlock) == 0x24);
#endif
