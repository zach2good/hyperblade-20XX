// The 32-byte animation set AnimSet::LoadIntoRegistry builds, and the base it shares with AnimDB.

#ifndef HYPERBLADE_ANIMCLIPSET_H
#define HYPERBLADE_ANIMCLIPSET_H

#include "animclip.h"

#include "anim/animobject.h"

// VTABLE: HYPERX 0x004b22a0
class AnimClipSet : public AnimObject
{
public:
    ~AnimClipSet() override;

    // AnimSet::LoadIntoRegistry files it in the registry's set table.
    AnimClipSet(FileObj* pFile, const char* pszName)
    {
        Load(pFile, pszName);
    }

    void Init();
    void Load(FileObj* pFile, const char* pszName);

    short m_nClipCount; // 0x08
    char* m_pszName;    // 0x0c
    // Copied from the file header; the clip pointer sits between the third and fourth.
    short     m_wFrameCount; // 0x10
    short     m_wFrameRate;  // 0x12
    float     m_flDuration;  // 0x14
    AnimClip* m_pClips;      // 0x18
    short     m_wUnused;     // 0x1c
    short     m_wEndFrame;   // 0x1e, the bound the binder clamps a node's end frame to
};

// SYNTHETIC: HYPERX 0x00417960
// AnimClipSet::`scalar deleting destructor'

// The length AnimSet::LoadIntoRegistry asks operator new for.
#endif
