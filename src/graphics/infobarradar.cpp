#include "common/compat.h"
#include <algorithm>
#include "math/point.h"
#include "engine/objectclasses.h"
#include "infobarradar.h"
#include "graphics/cdddrawable.h"
#include "math/vector.h"
#include "engine/devicector.h"
#include "engine/cameraobject.h"
#include "globals.h"
#include "engine/gameengine.h"
#include "physics/physicsbody.h"
#include "engine/modelids.h"
#include <cmath>
#include "meshbone.h"
#include "rlespritesurface.h"
#include "graphics/swrend.h"

// The points are zeroed by the helper and then again field by field, as in the original.
// FUNCTION: HYPERX 0x0044fcd0
InfoBarRadar::InfoBarRadar()
{
    m_ptDrawOffset.y = 0;
    m_ptDrawOffset.x = 0;
    m_ptPosition.y   = 0;
    m_ptPosition.x   = 0;
    m_ptSize.y       = 0;
    m_ptSize.x       = 0;

    m_nUnused  = 0;
    m_flScaleY = 0.0f;
    m_flScaleX = 0.0f;

    m_vPosition.x = m_vPosition.y = m_vPosition.z = 0.0f;

    m_flCos = 0.0f;
    m_flSin = 0.0f;

    m_nSlot = kModelIDNone;
}

// FUNCTION: HYPERX 0x0044fd60
InfoBarRadar::~InfoBarRadar() noexcept = default;

// The limits and a third, unread value arrive as one twelve-byte block.
// FUNCTION: HYPERX 0x0044fd70
void InfoBarRadar::ComputeScale(const CDDDrawable* pFrame, Vec3 vLimits)
{
    float flDivisor;

    const auto nHeight = pFrame->m_ptSize.y;

    const auto nHalfWidth = (pFrame->m_ptSize.x - 4) / 2;
    m_ptDrawOffset.x      = nHalfWidth;

    const auto nHalfHeight = (nHeight - 4) / 2;
    m_ptDrawOffset.y       = nHalfHeight;

    flDivisor = (std::max)(vLimits.x, vLimits.y);

    m_flScaleX = static_cast<float>(nHalfWidth) / flDivisor;
    m_flScaleY = static_cast<float>(nHalfHeight) / flDivisor;
}

// FUNCTION: HYPERX 0x0044fdd0
int InfoBarRadar::TrackSlot(int nSlot)
{
    auto* const pObject = g_pGameEngine->m_ppSlots[static_cast<short>(nSlot)];

    if (pObject == nullptr)
    {
        return 0;
    }

    m_nSlot = nSlot;

    m_vPosition = *pObject->GetPositionPtr();

    auto* const pHeading = g_pGameEngine->GetCamera(kCameraMainSlot);

    if (pHeading == nullptr)
    {
        return 0;
    }

    auto vDirection = pHeading->m_dirOut;

    const auto dAngle = atan2(static_cast<double>(vDirection.x), static_cast<double>(vDirection.y));

    m_flSin = static_cast<float>(sin(static_cast<double>(dAngle)));
    m_flCos = static_cast<float>(cos(static_cast<double>(dAngle)));

    return 1;
}

// FUNCTION: HYPERX 0x0044fe70
void InfoBarRadar::DrawBlip(int nSlot, int bTrail)
{
    Point ptCentre;
    Vec3  vDelta;
    int   nColour;

    if (nSlot == m_nSlot)
    {
        return;
    }

    switch (nSlot)
    {
        case kSlotDromer0:
            nColour = g_nRadarBlipColourDromer0;
            break;

        case kSlotDromer2:
            nColour = g_nRadarBlipColourDromer2;
            break;

        case kSlotDromer1:
            nColour = g_nRadarBlipColourDromer1;
            break;

        case kSlotDromer3:
            nColour = g_nRadarBlipColourDromer3;
            break;

        case kSlotRok:
            nColour = g_nRadarBlipColourRok;
            break;

        default:
            return;
    }

    ptCentre.Zero();

    ptCentre.x = m_ptDrawOffset.x + m_ptPosition.x;
    ptCentre.y = m_ptDrawOffset.y + m_ptPosition.y;

    auto* const pObject = g_pGameEngine->m_ppSlots[static_cast<short>(nSlot)];

    if (pObject == nullptr)
    {
        return;
    }

    auto* const pPosition = pObject->GetPositionPtr();

    vDelta.x = pPosition->x - m_vPosition.x;
    vDelta.y = pPosition->y - m_vPosition.y;

    if (vDelta.x <= 170.0f && vDelta.x >= -170.0f && vDelta.y <= 170.0f && vDelta.y >= -170.0f)
    {
        const auto flX = vDelta.x * m_flScaleX;
        const auto flY = vDelta.y * m_flScaleY;

        const auto nX = static_cast<int>(flX * m_flCos - flY * m_flSin) + ptCentre.x;
        const auto nY = ptCentre.y - static_cast<int>((flX * m_flSin) + flY * m_flCos);

        SWRend::FillRect(nX, nY, 4, 4, nColour);

        if (bTrail != 0)
        {
            SWRend::DrawLine(ptCentre.x + 2, ptCentre.y + 2, nX + 2, nY + 2, g_nRadarLineColour);
        }
    }
}

constexpr auto kArrowPoints = 3;

// FUNCTION: HYPERX 0x00450000
void InfoBarRadar::DrawMarker(int)
{
    Point apt[kArrowPoints];

    apt[0].x = 0;

    const auto nOriginX = m_ptDrawOffset.x + m_ptPosition.x + 2;
    const auto nOriginY = m_ptDrawOffset.y + m_ptPosition.y + 2;

    if ((m_nSlot & 1) != 0)
    {
        apt[0].y = 0xbe;
        apt[1].x = -0xf;
        apt[1].y = 0x8c;
        apt[2].x = 0xf;
        apt[2].y = 0x8c;
    }
    else
    {
        apt[0].y = -0xbe;
        apt[1].x = 0xf;
        apt[1].y = -0x8c;
        apt[2].x = -0xf;
        apt[2].y = -0x8c;
    }

    for (int i = 0; i < kArrowPoints; i++)
    {
        const auto flX = static_cast<float>(apt[i].x) * m_flScaleX;
        const auto flY = static_cast<float>(apt[i].y) * m_flScaleY;

        apt[i].x = static_cast<int>(flX * m_flCos - flY * m_flSin) + nOriginX;
        apt[i].y = nOriginY - static_cast<int>(flX * m_flSin + flY * m_flCos);
    }

    SWRend::FillPolygon(apt, kArrowPoints, 0xe6);
}

// The marker is drawn only when slot six tracks the engine's slot; the four low slots draw no line.
// FUNCTION: HYPERX 0x004500e0
int InfoBarRadar::Draw()
{
    PhysicsBody* pBlock;

    const int nSlot = g_pGameEngine->m_nViewedSlot;

    if (TrackSlot(nSlot) != 0)
    {
        pBlock = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody();

        auto* pTracked = pBlock->pAttach;

        if (pTracked != nullptr && pTracked->nSlot < kPlayerObjectCount && pTracked->nSlot == nSlot)
        {
            // The odd team's marker is frame seven, the even team's eight.
            int nFrame;

            if ((pTracked->nSlot & 1) != 0)
            {
                nFrame = 7;
            }
            else
            {
                nFrame = 8;
            }

            DrawMarker(nFrame);
        }
        else
        {
            DrawBlip(kSlotRok, 1);
        }

        for (int nOther = kSlotDromer0; nOther <= kSlotDromer3; nOther++)
        {
            DrawBlip(nOther, 0);
        }

        return 1;
    }

    return 0;
}
