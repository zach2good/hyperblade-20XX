#ifndef HYPERBLADE_MOVEPOSITIONWORD_H
#define HYPERBLADE_MOVEPOSITIONWORD_H

// Word A: x and y, ten bits of magnitude and a sign each, then z's magnitude in the top ten bits.
struct MovePositionWord
{
    unsigned int nWord;

    int GetX() const
    {
        return nWord & 0x3ff;
    }

    unsigned int GetXSign() const
    {
        return nWord & 0x400;
    }

    int GetY() const
    {
        return (nWord >> 11) & 0x3ff;
    }

    unsigned int GetYSign() const
    {
        return nWord & 0x200000;
    }

    int GetZ() const
    {
        return nWord >> 22;
    }

    // Each component is eleven bits of magnitude and sign; z's sign goes to word B.
    void Set(unsigned int nX, unsigned int nY, unsigned int nZ)
    {
        nWord = ((nY & 0x7ff) | (nZ << 11)) << 11 | (nX & 0x7ff);
    }
};

#endif
