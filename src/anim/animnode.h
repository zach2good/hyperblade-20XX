#ifndef HYPERBLADE_ANIMNODE_H
#define HYPERBLADE_ANIMNODE_H

#include "common/compat.h"

class AnimClipSet;

#define DESCRIPTION_CHARS 0x1f

// One entry of a script database's clip table: name, frame span and three rates.
class AnimNode
{
public:
    explicit AnimNode(char bHasDescription);

    int m_nUnused; // 0x00
    // The name is 20 bytes: animregistry.cpp writes FindOrLoad's answer to m_pSet.
    char               m_szName[0x14];   // 0x04
    class AnimClipSet* m_pSet;           // 0x18
    char*              m_pszDescription; // 0x1c
    short              m_wStartFrame;    // 0x20
    short              m_wEndFrame;      // 0x22
    float              m_flSlowFps;      // 0x24
    float              m_flStandardFps;  // 0x28
    float              m_flFastFps;      // 0x2c

    // %hd sets the low half and nothing clears the high half, as in the original; readers take both widths.
    union
    {
        int   m_nLoopCount; // 0x30
        short m_wLoopCount;
    };
};

#endif
