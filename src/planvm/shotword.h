#ifndef HYPERBLADE_SHOTWORD_H
#define HYPERBLADE_SHOTWORD_H

// kMsgShoot's argument: the shot power above the head camera's aim across and up, each a signed
// byte.
struct ShotWord
{
    unsigned int nWord;

    int GetPower() const
    {
        return static_cast<char>(nWord >> 16);
    }

    int GetAimX() const
    {
        return static_cast<char>(nWord >> 8);
    }

    int GetAimY() const
    {
        return static_cast<char>(nWord);
    }

    // Each byte is sign-extended before the OR, as the client packs it.
    static unsigned int Pack(int nAimX, int nPower, int nAimY)
    {
        return ((nAimX | (nPower << 8)) << 8) | nAimY;
    }
};

#endif
