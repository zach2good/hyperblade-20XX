#ifndef HYPERBLADE_MOVESCALEFLAGS_H
#define HYPERBLADE_MOVESCALEFLAGS_H

// The flags byte, widened: velocity z's top magnitude bit and sign, then the two scale shifts.
struct MoveScaleFlags
{
    unsigned int nFlags;

    int GetZHigh() const
    {
        return (nFlags & 1) << 9;
    }

    unsigned int GetZSign() const
    {
        return nFlags & 2;
    }

    int GetPositionShift() const
    {
        return (nFlags >> 2) & 7;
    }

    int GetVelocityShift() const
    {
        return (nFlags >> 5) & 7;
    }

    static unsigned char Pack(unsigned int nVelocityZ, unsigned int nPositionShift, unsigned int nVelocityShift)
    {
        return static_cast<unsigned char>(((nVelocityZ >> 9) & 3) | (nVelocityShift << 5) | ((nPositionShift & 7) << 2));
    }
};

#endif
