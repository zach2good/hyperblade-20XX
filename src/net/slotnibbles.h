#ifndef HYPERBLADE_SLOTNIBBLES_H
#define HYPERBLADE_SLOTNIBBLES_H

// The stationary objects' behaviours, two to a byte with the even slot in the low nibble.
struct SlotNibbles
{
    static int Get(const char* abNibbles, int nIndex)
    {
        return (abNibbles[nIndex / 2] >> ((nIndex & 1) * 4)) & 0x0f;
    }

    // The bits to OR into the slot's byte.
    static int Pack(int nIndex, int nBehaviour)
    {
        return (nBehaviour & 0xf) << ((nIndex & 1) << 2);
    }
};

#endif
