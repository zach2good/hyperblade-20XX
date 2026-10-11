#ifndef HYPERBLADE_MOVEVELOCITYWORD_H
#define HYPERBLADE_MOVEVELOCITYWORD_H

// Word B: the position's z sign, then x and y as in word A, then the low nine bits of z.
struct MoveVelocityWord
{
    unsigned int nWord;

    unsigned int GetPositionZSign() const
    {
        return nWord & 1;
    }

    int GetX() const
    {
        return (nWord >> 1) & 0x3ff;
    }

    unsigned int GetXSign() const
    {
        return nWord & 0x800;
    }

    int GetY() const
    {
        return (nWord >> 12) & 0x3ff;
    }

    unsigned int GetYSign() const
    {
        return nWord & 0x400000;
    }

    int GetZLow() const
    {
        return nWord >> 23;
    }

    // z's top magnitude bit and its sign go to the flags byte.
    void Set(unsigned int nX, unsigned int nY, unsigned int nZ, unsigned int nPositionZ)
    {
        nWord = (((nY & 0x7ff) | (nZ << 11)) << 11 | (nX & 0x7ff)) << 1 | ((nPositionZ >> 10) & 1);
    }
};

#endif
