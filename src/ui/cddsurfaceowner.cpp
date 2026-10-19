#include "common/compat.h"
#include "cddsurfaceowner.h"
#include <cstdlib>
#include "graphics/rlesprite.h"
#include "graphics/swrend.h"
#include "graphics/softrenderer.h"
#include "common/util.h"
#include <SDL3/SDL.h>
#include <cstring>

// FUNCTION: HYPERX 0x0044f8f0
int CDDSurfaceOwner::LoadByName(const char* pszName)
{
    if (m_pszName == nullptr || SDL_strcasecmp(pszName, m_pszName.get()) != 0)
    {
        m_pSprite = nullptr;

        m_pszName = std::make_unique<char[]>(strlen(pszName) + 1);

        strcpy(m_pszName.get(), pszName);

        auto pSprite = SoftRenderer::LoadRLESprite(pszName);

        if (pSprite != nullptr)
        {
            m_ptSize.x = pSprite->cx;
            m_ptSize.y = pSprite->cy;

            m_pSprite = std::move(pSprite);
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
    if (m_pSprite != nullptr)
    {
        SWRend::BlitRLESprite(m_ptDrawOffset.x + m_ptPosition.x, m_ptDrawOffset.y + m_ptPosition.y, m_nUnitWidth * m_nFrame, m_ptSize.y, m_pSprite.get());
    }

    return 1;
}
