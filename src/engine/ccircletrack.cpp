// Not an orbit: the angle at 0x004b1478 is zero, so the camera sits at a fixed offset.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "devicector.h"
#include "cameraobject.h"
#include "planvm/agendaitem.h"
#include "engine/ccircletrack.h"
#include "physics/collisionellipsoid.h"
#include "gameengine.h"
#include "globals.h"
#include "common/staticinit.h"
#include "math/vector.h"

#include <cmath>
#include "app/gameapp.h"
#include "engine/engineobject.h"
#include "engine/objectkind.h"

// FUNCTION: HYPERX 0x0040b6d0
void CCircleTrack::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CCircleTrack;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pWatchPos = &pOwner->GetPhysicsBody()->pos;
    m_pWatchDir = &pOwner->GetPhysicsBody()->velDir;
    m_nUnused   = g_pGameEngine->m_clock.nTickMs;
}

// FUNCTION: HYPERX 0x0040b720
void CCircleTrack::Process(int nArg)
{
    Vec3 vPos;
    Vec3 vTracked;
    Vec3 vZero;
    Vec3 vFarRay;
    Vec3 vNearRay;

    EngineObject* const pTracked  = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];
    auto* const         pCamera   = (CameraObject*)m_pOwner;
    const Vec3*         pSettings = pCamera->m_pSettings;

    if (pTracked == nullptr)
    {
        // STRING: HYPERX 0x004bb5dc
        g_gameApp.Fatal("CCircleTrack::Process(): Null pointer for model to track\n");
    }

    if (pTracked->m_nKind != KIND_DROMER)
    {
        // STRING: HYPERX 0x004bb5a0
        g_gameApp.Fatal("CCircleTrack::Process(): Model to track is not a dromer\n");
    }

    if (pCamera->GetTrackedObject() != pTracked)
    {
        pCamera->SetTrackedObject(pTracked);
    }

    PhysicsBody* pBody = pTracked->GetPhysicsBody();

    const double dRadius  = pSettings->x;
    const double dForward = pSettings->y + 12.0f;
    const double dCos     = cos(0.0);
    const double dSin     = sin(0.0);

    const Vec3* pTrackedPos = &pBody->pos;

    vTracked = *pTrackedPos;

    vPos.x = static_cast<float>(dCos * dRadius - dSin * dForward + vTracked.x);
    vPos.y = static_cast<float>(dSin * dRadius + dCos * dForward + vTracked.y);
    vPos.z = g_flCameraTrackingHeight + vTracked.z;

    if (vPos.x * vPos.x * g_camTrackEllipsoid.GetInvXX() + (vPos.y * vPos.y * g_camTrackEllipsoid.GetInvYY() + vPos.z * vPos.z * g_camTrackEllipsoid.GetInvZZ()) > 1.0)
    {
        Vec3* pClosest;

        vZero.x = g_flZeroTracking;
        vZero.y = g_flZeroTracking;
        vZero.z = g_flZeroTracking;

        g_camTrackEllipsoid.IntersectSegment(&vPos, &vZero, &vFarRay, &vNearRay);

        pClosest = Vec3::FindClosest(&vPos, &vFarRay, &vNearRay);

        vPos = *pClosest;
    }

    m_pWatchPos->x = vPos.x;
    m_pWatchPos->y = vPos.y;
    m_pWatchPos->z = vPos.z;

    m_pWatchDir->x = vTracked.x - m_pWatchPos->x;
    m_pWatchDir->y = vTracked.y - m_pWatchPos->y;
    m_pWatchDir->z = vTracked.z - m_pWatchPos->z;

    Math::Vec3Normalize(m_pWatchDir);

    pCamera->UpdateSmoothing();

    if (m_pNext != nullptr)
    {
        m_pNext->Process(nArg);
    }
}
