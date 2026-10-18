// A negative nBackIndex means no back material; a back material makes a face two sided.

#ifndef HYPERBLADE_MESHMATERIAL_H
#define HYPERBLADE_MESHMATERIAL_H

#include "graphics/materialname.h"

#include "anim/animclip.h"

#include <memory>

// The stride is twenty two, where natural packing gives twenty four.
#pragma pack(push, 2)

enum class FaceShade : unsigned char;

class FileObj;

struct MeshMaterial
{
    // D3DRend::DrawFace is unoptimised and still expands this constructor and the destructor inline.
    // FUNCTION: HYPERX 0x00436380
    inline MeshMaterial()
    {
        flAmbient  = 0.0f;
        flDiffuse  = 0.0f;
        flEmissive = 0.0f;
        pTexture   = nullptr;
        nBackIndex = -1;
        nColour    = 0;
    }

    inline ~MeshMaterial()
    {
    }

    unsigned char GetRed() const
    {
        return static_cast<unsigned char>(nColour >> 16);
    }

    unsigned char GetGreen() const
    {
        return static_cast<unsigned char>(nColour >> 8);
    }

    unsigned char GetBlue() const
    {
        return static_cast<unsigned char>(nColour);
    }

    // 0xAARRGGBB; the low channel is a palette index to the software paths.
    union
    {
        int           nColour;
        unsigned char acChannel[4];
    }; // 0x00

    float flAmbient;  // 0x04
    float flDiffuse;  // 0x08
    float flEmissive; // 0x0c

    // A one-based name index, replaced by the cache entry it loads.
    union
    {
        int                  nTextureIndex;
        class TexCacheEntry* pTexture;
    }; // 0x10

    short nBackIndex; // 0x14

    static FaceShade CalcLightIntensity(float* pflOut, const MeshMaterial* pMaterial, const struct Vec3* pNormal);

    static char ParseFile(FileObj* pFile, const char* pszModelName, std::unique_ptr<MeshMaterial[]>* ppMaterials, std::unique_ptr<MaterialName[]>* ppMatNames, std::unique_ptr<MaterialName[]>* ppTexNames, unsigned char* pnMatCount, short* pnTexCount);
};

// A material as a model file stores it, twenty two bytes with a one-based texture index.
struct MeshMaterialRecord
{
    int   nColour;
    float flAmbient;
    float flDiffuse;
    float flEmissive;
    int   nTextureIndex;
    short nBackIndex;
};

#pragma pack(pop)

static_assert(sizeof(MeshMaterialRecord) == 0x16);

enum class FaceShade : unsigned char
{
    kBlack  = 0,
    kWhite  = 1,
    kScaled = 2
};

#endif
