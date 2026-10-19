// The animation database; its base AnimObject is the eight-byte class at 0x004b22a4.

#ifndef HYPERBLADE_ANIMDB_H
#define HYPERBLADE_ANIMDB_H

class AnimClipEntry;

#include "animclipset.h"
#include <memory>
#include <string.h>

class AnimNode;

// VTABLE: HYPERX 0x004b22a8
class AnimDB : public AnimObject
{
public:
    AnimDB(const char* pszName, const char* pszExtra);
    ~AnimDB() override;

    AnimDB(const AnimDB&)            = delete;
    AnimDB& operator=(const AnimDB&) = delete;
    AnimDB(AnimDB&&)                 = delete;
    AnimDB& operator=(AnimDB&&)      = delete;

    void SetName(const char* pszName);

    void AddClipEntry(unsigned short wSegment, unsigned short wTriggerFrame, const char* pszName, int nMessage, const char* pszRef, short nSyncSegment, short nSyncFrame);

    // A record the anim player and the registry walk, so the layout is public.

    // The name the two registry walks compare; read from outside this class.
    const char* GetName() const
    {
        return m_szName;
    }

    short GetNodeCount() const
    {
        return m_wNodeCount;
    }

    short GetEntryCount() const
    {
        return m_nEntryCount;
    }

    class AnimClipEntry* GetEntry(short nIndex) const
    {
        return m_apEntries[nIndex].get();
    }

    class AnimNode* GetNode(short nIndex) const
    {
        return m_apNodes[nIndex].get();
    }

    // AnimDB::SetName copies at most 20 bytes here.
    char m_szName[20]; // 0x08
    // Always null; the original's destructor frees it, and nothing allocates it.
    void* m_pOwnedBlock; // 0x1c
    // Six nodes.
    short                     m_wNodeCount; // 0x20
    std::unique_ptr<AnimNode> m_apNodes[6]; // 0x24
    // Ten clip entries and their count; AddClipEntry appends at the count.
    std::unique_ptr<AnimClipEntry> m_apEntries[10]; // 0x3c
    short                          m_nEntryCount;   // 0x64
};

// SYNTHETIC: HYPERX 0x004175a0
// AnimDB::`scalar deleting destructor'

#endif
