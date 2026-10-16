// Two vectors in nine bytes: ten-bit components with a sign bit and a shared power-of-two scale.

#include "math/floatbits.h"
#include "common/compat.h"
#include "net/movecodec.h"
#include "net/movescaleflags.h"

// FUNCTION: HYPERX 0x0042bef0
void MoveCodec::Decode(const PackedMove* pIn, Vec3* pPosition, Vec3* pVelocity)
{
    MoveVelocityWord velocity;

    const MovePositionWord position = pIn->position;
    const MoveScaleFlags   flags    = { pIn->cFlags };

    int nPosX = position.GetX();
    int nPosY = position.GetY();
    int nPosZ = position.GetZ();

    if (position.GetXSign() != 0)
    {
        nPosX = -nPosX;
    }

    if (position.GetYSign() != 0)
    {
        nPosY = -nPosY;
    }

    velocity = pIn->velocity;

    if (velocity.GetPositionZSign() != 0)
    {
        nPosZ = -nPosZ;
    }

    const int nPosShift = flags.GetPositionShift();
    int       nVelX     = velocity.GetX();
    int       nVelY     = velocity.GetY();
    int       nVelZ     = velocity.GetZLow() | flags.GetZHigh();

    if (velocity.GetXSign() != 0)
    {
        nVelX = -nVelX;
    }

    if (velocity.GetYSign() != 0)
    {
        nVelY = -nVelY;
    }

    if (flags.GetZSign() != 0)
    {
        nVelZ = -nVelZ;
    }

    pPosition->x = (float)nPosX / (float)(1 << (9 - nPosShift));
    pPosition->y = (float)nPosY / (float)(1 << (9 - nPosShift));
    pPosition->z = (float)nPosZ / (float)(1 << (9 - nPosShift));

    pVelocity->x = (float)nVelX / (float)(1 << (9 - flags.GetVelocityShift()));
    pVelocity->y = (float)nVelY / (float)(1 << (9 - flags.GetVelocityShift()));
    pVelocity->z = (float)nVelZ / (float)(1 << (9 - flags.GetVelocityShift()));
}

// A component shifted past 31 bits is stored as zero; an out-of-range exponent refuses the move.

// FUNCTION: HYPERX 0x0042c030
int MoveCodec::Encode(PackedMove* pOut, const Vec3* pPosition, const Vec3* pVelocity)
{
    unsigned int nPosExp;
    unsigned int nPosExpX;
    unsigned int nPosExpY;
    unsigned int nPosExpZ;
    unsigned int nVelExp;
    unsigned int nVelExpX;
    unsigned int nVelExpY;
    unsigned int nVelExpZ;
    unsigned int nPosX;
    unsigned int nPosY;
    unsigned int nPosZ;
    unsigned int nVelX;
    unsigned int nVelY;
    unsigned int nVelZ;
    unsigned int nPosBitsX;
    unsigned int nPosBitsY;
    unsigned int nPosBitsZ;
    unsigned int nVelBitsX;
    unsigned int nVelBitsY;
    unsigned int nVelBitsZ;

    nPosBitsX = FloatBits(pPosition->x);
    nPosBitsY = FloatBits(pPosition->y);
    nPosBitsZ = FloatBits(pPosition->z);

    nPosX = (nPosBitsX & 0x7fffff) | 0x800000;
    nPosY = (nPosBitsY & 0x7fffff) | 0x800000;
    nPosZ = (nPosBitsZ & 0x7fffff) | 0x800000;

    nPosExpX = (nPosBitsX >> 23) & 0xff;
    nPosExpY = (nPosBitsY >> 23) & 0xff;
    nPosExpZ = (nPosBitsZ >> 23) & 0xff;

    nPosExp = nPosExpX;

    if (nPosExp < nPosExpY)
    {
        nPosExp = nPosExpY;
    }

    if (nPosExp < nPosExpZ)
    {
        nPosExp = nPosExpZ;
    }

    if (nPosExp < 0x7f)
    {
        nPosExp = 0x7f;
    }
    else if (nPosExp > 0x86)
    {
        return 0;
    }

    nPosX >>= (nPosExp - nPosExpX) + 14;

    nPosY >>= (nPosExp - nPosExpY) + 14;

    nPosZ >>= (nPosExp - nPosExpZ) + 14;

    if ((nPosExp - nPosExpX) + 14 > 31)
    {
        nPosX = 0;
    }

    if ((nPosExp - nPosExpY) + 14 > 31)
    {
        nPosY = 0;
    }

    if ((nPosExp - nPosExpZ) + 14 > 31)
    {
        nPosZ = 0;
    }

    if (nPosX != 0)
    {
        nPosX |= (nPosBitsX >> 21) & 0x400;
    }

    if (nPosY != 0)
    {
        nPosY |= (nPosBitsY >> 21) & 0x400;
    }

    if (nPosZ != 0)
    {
        nPosZ |= (nPosBitsZ >> 21) & 0x400;
    }

    const unsigned int nPosScale = nPosExp - 0x7f;

    nVelBitsX = FloatBits(pVelocity->x);
    nVelBitsY = FloatBits(pVelocity->y);
    nVelBitsZ = FloatBits(pVelocity->z);

    nVelX = (nVelBitsX & 0x7fffff) | 0x800000;
    nVelY = (nVelBitsY & 0x7fffff) | 0x800000;
    nVelZ = (nVelBitsZ & 0x7fffff) | 0x800000;

    nVelExpX = (nVelBitsX >> 23) & 0xff;
    nVelExpY = (nVelBitsY >> 23) & 0xff;
    nVelExpZ = (nVelBitsZ >> 23) & 0xff;

    nVelExp = nVelExpX;

    if (nVelExp < nVelExpY)
    {
        nVelExp = nVelExpY;
    }

    if (nVelExp < nVelExpZ)
    {
        nVelExp = nVelExpZ;
    }

    if (nVelExp < 0x7f)
    {
        nVelExp = 0x7f;
    }
    else if (nVelExp > 0x86)
    {
        return 0;
    }

    nVelX >>= (nVelExp - nVelExpX) + 14;

    nVelY >>= (nVelExp - nVelExpY) + 14;

    nVelZ >>= (nVelExp - nVelExpZ) + 14;

    if ((nVelExp - nVelExpX) + 14 > 31)
    {
        nVelX = 0;
    }

    if ((nVelExp - nVelExpY) + 14 > 31)
    {
        nVelY = 0;
    }

    if ((nVelExp - nVelExpZ) + 14 > 31)
    {
        nVelZ = 0;
    }

    if (nVelX != 0)
    {
        nVelX |= (nVelBitsX >> 21) & 0x400;
    }

    if (nVelY != 0)
    {
        nVelY |= (nVelBitsY >> 21) & 0x400;
    }

    if (nVelZ != 0)
    {
        nVelZ |= (nVelBitsZ >> 21) & 0x400;
    }

    pOut->position.Set(nPosX, nPosY, nPosZ);

    pOut->velocity.Set(nVelX, nVelY, nVelZ, nPosZ);

    pOut->cFlags = MoveScaleFlags::Pack(nVelZ, nPosScale, nVelExp - 0x7f);

    return 1;
}
