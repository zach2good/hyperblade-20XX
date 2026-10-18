#include "common/compat.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "softtex.h"
#include "common/pathstring.h"
#include "texcacheentry.h"
#include "engine/gameengine.h"
#include "planvm/planvm.h"
#include <new>
#include <cstring>
#include "app/gameapp.h"
#include "common/stringpool.h"
#include "app/stringids.h"

// On failure the entry stays in the cache with a null texture.

// FUNCTION: HYPERX 0x00492b90
TexCacheEntry::TexCacheEntry(const char* pszName)
{
    m_nUnused = 0;
    m_wRefs   = 0;
    m_pNext   = nullptr;

    m_pszName = strcpy(new char[strlen(pszName) + 1], pszName);

    SoftTex* pTexture;

    pTexture = new SoftTex;

    if (pTexture != nullptr && pTexture->Load(pszName) == 0)
    {
        delete pTexture;
        pTexture = nullptr;
    }

    m_pSoftTexture = pTexture;
}

// FUNCTION: HYPERX 0x00492d70
void* TexCacheEntry::DestroyTexObj()
{
    delete m_pSoftTexture;

    m_pSoftTexture = nullptr;

    return m_pSoftTexture;
}

// FUNCTION: HYPERX 0x00492dc0
TexCacheEntry::~TexCacheEntry()
{
    if (m_pszName != nullptr)
    {
        delete[] m_pszName;
        m_pszName = nullptr;
    }

    DestroyTexObj();
}
