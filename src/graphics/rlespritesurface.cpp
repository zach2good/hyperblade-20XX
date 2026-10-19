#include "common/compat.h"
#include "rlespritesurface.h"
#include "rlesprite.h"
#include <cstring>
#include "common/commonglobals.h"
#include "graphics/infobarradar.h"
#include "ui/cddsurfaceowner.h"
#include "graphics/swrend.h"
#include "graphics/softrenderer.h"
#include "common/staticinit.h"
#include <SDL3/SDL.h>

extern const float g_flZeroSurface2    = 0.0f;
float              g_flUnusedSurface2A = g_flZeroSurface2;
float              g_flUnusedSurface2B = g_flZeroSurface2;
float              g_flUnusedSurface2C = g_flZeroSurface2;

// The multibyte compare. See library.h.

// FUNCTION: HYPERX 0x0044f140
RLESpriteSurface::~RLESpriteSurface() = default;

// FUNCTION: HYPERX 0x0044f210
int RLESpriteSurface::Draw()
{
    if (m_pSprite != nullptr)
    {
        SWRend::BlitRLESprite(m_ptDrawOffset.x + m_ptPosition.x, m_ptDrawOffset.y + m_ptPosition.y, m_ptSize.x, m_ptSize.y, m_pSprite.get());
    }

    return 1;
}

// CDDSurfaceOwner::LoadByName is the same body over different fields.
// FUNCTION: HYPERX 0x0044f3c0
int RLESpriteSurface::LoadByName(const char* pszName)
{
    if (m_strName.empty() || SDL_strcasecmp(pszName, m_strName.c_str()) != 0)
    {
        m_pSprite = nullptr;
        m_strName = pszName;

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
