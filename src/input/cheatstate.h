#ifndef HYPERBLADE_CHEATSTATE_H
#define HYPERBLADE_CHEATSTATE_H

class CheatState
{
public:
    int ProcessKeypress(char cKey);

    // ProcessKeypress shifts the history down and writes the newest key in the last byte.
    char m_szHistory[32];
};

#endif
