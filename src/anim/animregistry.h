// The animation registry; its script table, databases and sets are told apart by size.

#ifndef HYPERBLADE_ANIMREGISTRY_H
#define HYPERBLADE_ANIMREGISTRY_H

#include "anim/animscripttable.h"

// The registry holds at most this many sets.
#define ANIM_REGISTRY_SET_LIMIT 250

// The registry itself, 0x10 bytes; Game::InitializeEngine builds the one there is.
class AnimRegistry
{
public:
    AnimRegistry()
    {
        m_nLoadCost = 0;
        m_nSetCount = 0;
        m_apSets    = new class AnimClipSet*[ANIM_REGISTRY_SET_LIMIT];

        m_pScripts = new AnimScriptTable;
    }

    void LoadFromPath(const char* pszPath);

    class AnimClipSet* FindInCache(const char* pszName);
    class AnimClipSet* FindOrLoad(const char* pszName);

    // One script database, given as the argument rather than as the receiver.
    class AnimDB* BindAnimSets(class AnimDB* pDb);

    // Both compare case-insensitively; no match is null and minus one.
    class AnimDB* FindAnimByNamePtr(const char* pszName);
    short         FindAnimByNameIdx(const char* pszName);

    int m_nLoadCost; // 0x00
    int m_nSetCount; // 0x04
    // The loaded sets, m_nSetCount of them, in a thousand-byte buffer.
    class AnimClipSet**    m_apSets;   // 0x08
    class AnimScriptTable* m_pScripts; // 0x0c

    // Deletes every loaded set, the table and the script table.
    ~AnimRegistry();
};

#endif
