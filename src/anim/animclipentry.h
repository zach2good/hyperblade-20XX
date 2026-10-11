// The entry an animation database keeps for one clip.

#ifndef HYPERBLADE_ANIMCLIPENTRY_H
#define HYPERBLADE_ANIMCLIPENTRY_H

#include "common/animnamedref.h"
#include <string.h>
#include "common/staticassert.h"

class AnimClipEntry
{
public:
    AnimClipEntry(unsigned short wSegment, unsigned short wTriggerFrame, const char* pszName, int nMessage);

    short          m_wSegment;      // 0x00
    unsigned short m_wTriggerFrame; // 0x02
    char           m_szName[0x18];  // 0x04
    int            m_nMessage;      // 0x1c
    AnimNamedRef*  m_pRef;          // 0x20

    // animregistry.cpp calls this on every entry of the animation kind.
    AnimClipEntry* SyncCheck(class AnimDB* pDb);
};

inline AnimClipEntry::AnimClipEntry(unsigned short wSegment, unsigned short wTriggerFrame, const char* pszName, int nMessage)
{
    m_wSegment      = wSegment;
    m_wTriggerFrame = wTriggerFrame;
    m_nMessage      = nMessage;
    m_pRef          = 0;

    strcpy(m_szName, pszName);
}

STATIC_ASSERT(sizeof(AnimClipEntry) == 0x24);
#endif
