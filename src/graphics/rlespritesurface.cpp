#include "common/compat.h"
#include "rlespritesurface.h"
#include "rlesprite.h"
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include "common/commonglobals.h"
#include "graphics/infobarradar.h"
#include "ui/cddsurfaceowner.h"
#include "graphics/swrend.h"
#include "graphics/softrenderer.h"
#include "common/util.h"
#include "common/staticinit.h"

extern const float g_flZeroSurface2    = 0.0f;
float              g_flUnusedSurface2A = g_flZeroSurface2;
float              g_flUnusedSurface2B = g_flZeroSurface2;
float              g_flUnusedSurface2C = g_flZeroSurface2;

// The multibyte compare. See library.h.

// FUNCTION: HYPERX 0x0044f140
RLESpriteSurface::~RLESpriteSurface()
{
    if (m_pSprite != 0)
    {
        Util::SafeFree(m_pSprite);
    }

    if (m_pszName != 0)
    {
        free(m_pszName);
    }
}

// FUNCTION: HYPERX 0x0044f210
int RLESpriteSurface::Draw()
{
    if (m_pSprite != 0)
    {
        SWRend::BlitRLESprite(m_ptDrawOffset.x + m_ptPosition.x, m_ptDrawOffset.y + m_ptPosition.y, m_ptSize.x, m_ptSize.y, m_pSprite);
    }

    return 1;
}

// CDDSurfaceOwner::LoadByName is the same body over different fields.
// FUNCTION: HYPERX 0x0044f3c0
int RLESpriteSurface::LoadByName(const char* pszName)
{
    if (m_pszName == 0 || _mbsicmp((const unsigned char*)pszName, (const unsigned char*)m_pszName) != 0)
    {
        if (m_pSprite != 0)
        {
            Util::SafeFree(m_pSprite);

            m_pSprite = 0;
        }

        if (m_pszName != 0)
        {
            free(m_pszName);
        }

        char* const pszCopy = (char*)malloc(strlen(pszName) + 1);

        if (pszCopy != 0)
        {
            strcpy(pszCopy, pszName);
        }

        m_pszName = pszCopy;

        RLESprite* const pSprite = SoftRenderer::LoadRLESprite(pszName);

        if (pSprite != 0)
        {
            m_pSprite = pSprite;

            m_ptSize.x = pSprite->cx;
            m_ptSize.y = pSprite->cy;
        }
        else
        {
            m_ptSize.x = 0;
            m_ptSize.y = 0;

            return 0;
        }
    }

    return 1;
}
