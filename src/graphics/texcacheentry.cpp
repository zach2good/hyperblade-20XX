#include "common/compat.h"
#include "d3ddeviceinit.h"
#include "graphics/texture.h"
#include "graphics/cd3dtexture.h"
#include "globals.h"
#include "graphics/renderstate.h"
#include "graphicsglobals.h"
#include "softtex.h"
#include "common/pathstring.h"
#include "texcacheentry.h"
#include "d3ddevice.h"
#include "engine/gameengine.h"
#include "planvm/planvm.h"
#include <new.h>
#include <string.h>
#include "app/gameapp.h"
#include "common/stringpool.h"
#include "app/stringids.h"

// On failure the entry stays in the cache with a null texture. Only the Direct3D path complains.

// FUNCTION: HYPERX 0x00492b90
TexCacheEntry::TexCacheEntry(const char* pszName, HPALETTE hPalette)
{
    m_nUnused = 0;
    m_bIsD3D  = 0;
    m_wRefs   = 0;
    m_pNext   = 0;

    m_pszName = strcpy(new char[strlen(pszName) + 1], pszName);

    m_bIsD3D = (g_cD3DActive != 0 && g_d3dState.bD3DTexturesEnabled != 0);

    if (m_bIsD3D != 0)
    {
        CD3DTexture* pTexture;
        PathString   path(MAX_PATH, g_gameApp.m_strTexturesPath.m_str.m_pchData, pszName, g_pszBitmapExtension);

        pTexture = new CD3DTexture(g_ddrawDevice.pDDrawObject, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

        if (pTexture != 0 && pTexture->Create(path.m_str.m_pchData, 0, 1) == 0)
        {
            delete pTexture;
            pTexture = 0;
        }

        if (pTexture == 0)
        {
            g_gameApp.Fatal(g_gameApp.m_stringPool.LoadText(STRING_TEXTURE_LOAD_FAILED));
        }

        m_pD3DTexture = pTexture;
    }
    else
    {
        SoftTex* pTexture;

        pTexture = new SoftTex;

        if (pTexture != 0 && pTexture->Load(pszName, hPalette) == 0)
        {
            delete pTexture;
            pTexture = 0;
        }

        m_pSoftTexture = pTexture;
    }
}

// FUNCTION: HYPERX 0x00492d70
void* TexCacheEntry::DestroyTexObj()
{
    CD3DTexture* const pD3DTexture = m_pD3DTexture;

    if (pD3DTexture != 0)
    {
        if (m_bIsD3D != 0)
        {
            delete pD3DTexture;
        }
        else
        {
            delete m_pSoftTexture;
        }

        m_pD3DTexture = 0;
    }

    return m_pD3DTexture;
}

// FUNCTION: HYPERX 0x00492dc0
TexCacheEntry::~TexCacheEntry()
{
    if (m_pszName != 0)
    {
        delete[] m_pszName;
        m_pszName = 0;
    }

    DestroyTexObj();
}
