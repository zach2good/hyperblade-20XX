#ifndef HYPERBLADE_MOVECODEC_H
#define HYPERBLADE_MOVECODEC_H

#include "math/vector.h"
#include "net/movepositionword.h"
#include "net/movevelocityword.h"

// Two vectors packed into nine bytes.
#pragma pack(push, 1)

struct PackedMove
{
    MovePositionWord position; // 0x00
    MoveVelocityWord velocity; // 0x04
    unsigned char    cFlags;   // 0x08
};

#pragma pack(pop)

static_assert(sizeof(PackedMove) == 9);

#include "common/compat.h"

namespace MoveCodec
{

void __cdecl Decode(const PackedMove* pIn, Vec3* pPosition, Vec3* pVelocity);
int __cdecl  Encode(PackedMove* pOut, const Vec3* pPosition, const Vec3* pVelocity);

} // namespace MoveCodec

#endif
