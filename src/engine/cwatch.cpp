// Both null checks report fatally and carry on, so a missing object crashes at the virtual call.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "planvm/agendaitem.h"
#include "engine/cwatch.h"
#include "cameraobject.h"
#include "devicector.h"
#include "gameengine.h"
#include "globals.h"
#include "engineglobals.h"
#include "math/vector.h"
#include "app/gameapp.h"
#include "engine/engineobject.h"

// FUNCTION: HYPERX 0x0040ad50
void CWatch::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind  = Agenda_CWatch;
    m_pNext  = nullptr;
    m_bReady = 0;
    m_pOwner = pSubject;
}

// FUNCTION: HYPERX 0x0040ad70
void CWatch::Process(int nArg)
{
    Vec3 vDir;

    auto* const pWatched = GetCamera()->GetSecondaryTrackedObject();
    auto* const pTracked = GetCamera()->GetTrackedObject();

    if (pWatched == nullptr)
    {
        // STRING: HYPERX 0x004bb528
        g_gameApp.Fatal("CWatch::Process(): Null pointer for model to watch\n");
    }

    if (pTracked == nullptr)
    {
        // STRING: HYPERX 0x004bb4f4
        g_gameApp.Fatal("CWatch::Process(): Null pointer for model to track\n");
    }

    auto* const pWatchedPos = &pWatched->GetPhysicsBody()->pos;
    auto* const pTrackedPos = &pTracked->GetPhysicsBody()->pos;

    auto* const pCameraPos = &m_pOwner->GetPhysicsBody()->pos;
    auto* const pCameraDir = &m_pOwner->GetPhysicsBody()->velDir;

    vDir.x = pWatchedPos->x - pTrackedPos->x;
    vDir.y = pWatchedPos->y - pTrackedPos->y;
    vDir.z = pWatchedPos->z - (GetCamera()->m_pSettings->z + pTrackedPos->z);

    Math::Vec3Normalize(&vDir);

    (&vDir)->ScaleInPlace(GetCamera()->m_pSettings->y);

    pCameraPos->x = pTrackedPos->x - vDir.x;
    pCameraPos->y = pTrackedPos->y - vDir.y;
    pCameraPos->z = (GetCamera()->m_pSettings->z + pTrackedPos->z) - vDir.z;

    pCameraDir->x = (pTrackedPos->x + pWatchedPos->x) * g_flCamWatchMidpointScale - pCameraPos->x;
    pCameraDir->y = (pTrackedPos->y + pWatchedPos->y) * g_flCamWatchMidpointScale - pCameraPos->y;
    pCameraDir->z = (pTrackedPos->z + pWatchedPos->z) * g_flCamWatchMidpointScale - pCameraPos->z;

    Math::Vec3Normalize(pCameraDir);

    GetCamera()->UpdateSmoothing();

    if (m_pNext != nullptr)
    {
        m_pNext->Process(nArg);
    }
}
