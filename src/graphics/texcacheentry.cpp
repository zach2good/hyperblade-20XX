#include "common/compat.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "softtex.h"
#include "common/pathstring.h"
#include "texcacheentry.h"
#include "engine/gameengine.h"
#include "planvm/planvm.h"
#include <cstring>
#include "app/gameapp.h"
#include "common/stringpool.h"
#include "app/stringids.h"

// On failure the entry stays in the cache with a null texture.

// FUNCTION: HYPERX 0x00492b90
TexCacheEntry::TexCacheEntry(const char* pszName)
: m_strName(pszName)
{
    m_nUnused = 0;
    m_wRefs   = 0;

    auto pTexture = std::make_unique<SoftTex>();

    if (pTexture->Load(pszName) == 0)
    {
        pTexture = nullptr;
    }

    m_pSoftTexture = std::move(pTexture);
}

// FUNCTION: HYPERX 0x00492d70
void* TexCacheEntry::DestroyTexObj()
{
    m_pSoftTexture = nullptr;

    return m_pSoftTexture.get();
}

// Unlinks the rest of the list one entry at a time, so a long list cannot exhaust the stack.
// FUNCTION: HYPERX 0x00492dc0
TexCacheEntry::~TexCacheEntry()
{
    auto pNext = std::move(m_pNext);

    while (pNext != nullptr)
    {
        pNext = std::move(pNext->m_pNext);
    }
}
