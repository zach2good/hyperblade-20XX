#ifndef HYPERBLADE_INPUT_CHEATTABLE_H
#define HYPERBLADE_INPUT_CHEATTABLE_H

// The first byte is the index of the last character, the second the exclusive-or key.
struct CheatEntry
{
    const char* pszCode;
    void (*pfnActivate)();
};

// The code table ends where the team order table begins.
// GLOBAL: HYPERX 0x004c28d0
extern CheatEntry g_aCheatCodes[15];

#endif
