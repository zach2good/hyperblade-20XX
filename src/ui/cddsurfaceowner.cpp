#include "common/compat.h"
#include "cddsurfaceowner.h"
#include <stdlib.h>
#include "graphics/rlesprite.h"
#include "graphics/swrend.h"
#include "graphics/softrenderer.h"
#include "common/util.h"
#include <SDL3/SDL.h>
#include <cstring>

// FUNCTION: HYPERX 0x0044f890
CDDSurfaceOwner::~CDDSurfaceOwner() throw()
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

// FUNCTION: HYPERX 0x0044f8f0
int CDDSurfaceOwner::LoadByName(const char* pszName)
{
    if (m_pszName == 0 || SDL_strcasecmp(pszName, m_pszName) != 0)
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

// FUNCTION: HYPERX 0x0044f9b0
int CDDSurfaceOwner::Draw()
{
    if (m_pSprite != 0)
    {
        SWRend::BlitRLESprite(m_ptDrawOffset.x + m_ptPosition.x, m_ptDrawOffset.y + m_ptPosition.y, m_nUnitWidth * m_nFrame, m_ptSize.y, m_pSprite);
    }

    return 1;
}
