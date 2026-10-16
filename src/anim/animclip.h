// The animation clip as the set's array sees it: twenty bytes with a constructor and a destructor.

#ifndef HYPERBLADE_ANIMCLIP_H
#define HYPERBLADE_ANIMCLIP_H

#include "math/vector.h"

// A format of one means the clip contributes no slerp segment.
#define ANIM_BONE_MODE_SKIP 1

class FileObj;
union MatrixRow;

// Packed, because the dword at 0x06 is not aligned.
#pragma pack(push, 1)

class AnimHeader
{
public:
    ~AnimHeader()
    {
    }

    short m_nClipCount;  // 0x00
    short m_nFrameCount; // 0x02
    // AnimClipSet::Load copies these two, and nothing reads the copies.
    short m_wFrameRate; // 0x04
    // The frame count over 120, whatever the frame rate.
    float m_flDuration; // 0x06
    // Copied into AnimClipSet::m_wUnused, which nothing reads.
    short m_wUnused; // 0x0a
    // Copied into AnimClipSet::m_wEndFrame, the bound the binder clamps a node's end frame to.
    short m_wEndFrame; // 0x0c
};

#pragma pack(pop)

static_assert(sizeof(AnimHeader) == 0x0e);

// One frame of a clip in formats seven and eight; the tick reads only the top-left three by three.
struct PackedBoneRow
{
    Vec3  vAxis;
    float flOffset;
};

struct PackedBoneMatrix
{
    PackedBoneRow aRows[4];
};

static_assert(sizeof(PackedBoneMatrix) == 0x40);

// One frame of a clip in formats five and six, which the loader allocates and never fills.
struct PackedQuaternion
{
    short aw[4];
};

static_assert(sizeof(PackedQuaternion) == 0x08);

// One frame of a clip in formats two to four: a three by three rotation in 16-bit fixed point.
struct PackedRotation
{
    unsigned short aw[3][3];
};

static_assert(sizeof(PackedRotation) == 0x12);

#pragma pack(push, 1)

// Only the five fields the loader reads are named; nothing reads the five bytes after the axes.
struct AnimClipHeader
{
    unsigned short wSegment;  // 0x00
    unsigned short wFrame;    // 0x02
    short          nDataSize; // 0x04
    // The Euler axis letters, applied last to first.
    char acAxisOrder[3]; // 0x06
    char acUnused[5];    // 0x09
    int  nFormat;        // 0x0e

    ~AnimClipHeader()
    {
    }
};

#pragma pack(pop)

// VTABLE: HYPERX 0x004b2238
class AnimClip
{
public:
    AnimClip();

    virtual ~AnimClip();

    unsigned char Load(FileObj* pFile, const AnimHeader* pSetHeader);

    // One frame's transform out of the packed stream, in the two shapes the players ask for.
    unsigned char GetBoneTransform(MatrixRow* pDest, short nFrame);
    unsigned char GetBoneTransformFull(MatrixRow* pDest, short nFrame);

    short m_nSegment; // 0x04
    short m_nFrame;   // 0x06

    // The rotation stream m_nFormat picks.
    union
    {
        PackedRotation*   m_pRotations; // 0x08
        PackedQuaternion* m_pQuaternions;
        PackedBoneMatrix* m_pMatrices;
    };

    Vec3* m_pTranslation; // 0x0c, one row per frame
    int   m_nFormat;      // 0x10
};

// Ghidra calls this class a plan condition expression; it is not one (docs/re_corrections.md).
// SYNTHETIC: HYPERX 0x00417440
// AnimClip::`vector deleting destructor'

#endif
