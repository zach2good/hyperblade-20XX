#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "engine/cameraobject.h"
#include "globals.h"
#include "engine/engineglobals.h"
#include "math/vector.h"
#include "engine/cheadtrack.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "engine/gameengine.h"
#include "planvm/agendaitem.h"
#include "math/transformobject.h"
#include "graphics/meshbone.h"
#include "engine/objectstats.h"
#include "common/staticinit.h"
#include "damagegate.h"
#include "engine/objectclasses.h"
#include "engine/playerobject.h"
#include "engine/engineobject.h"
#include "math/point.h"
#include "graphics/viewextent.h"
#include "engine/statids.h"

// The broadcast camera: the helmet view a spectator watches the match through.

#define HISTORY_WINDOW_OFF 1

// FUNCTION: HYPERX 0x00409fc0
void CHeadTrack::Bind(EngineObject* pCamera, GameBehaviorObject*)
{
    m_pNext     = nullptr;
    m_wKind     = Agenda_CHeadTrack;
    m_bReady    = 0;
    m_pOwner    = pCamera;
    m_pWatchPos = &pCamera->GetPhysicsBody()->pos;
    m_pWatchDir = &pCamera->GetPhysicsBody()->velDir;

    auto* const pCam = (CameraObject*)m_pOwner;

    pCam->SetViewFracX(g_flCamWatchMidpointScale);

    pCam->SetViewFracY(g_flCamWatchMidpointScale);

    m_bAttached = 0;

    if (g_pRenderer != nullptr)
    {
        ViewExtent size;
        Point      origin;
        double     dFracX;
        double     dFracY;

        size = g_pRenderer->GetHeadCamSize();

        size.ShrinkClamp(g_pRenderer->GetReticleSize());

        origin.Zero();

        dFracX   = pCam->m_flViewFracX;
        origin.x = (int)(dFracX * size.x);

        dFracY   = pCam->m_flViewFracY;
        origin.y = (int)(dFracY * size.y);

        g_pRenderer->SetReticlePos(origin.x, origin.y);
    }

    pCam->SetTrackedObject(g_pGameEngine->m_ppSlots[0]);

    pCam->SetPosHistorySize(HISTORY_WINDOW_OFF);
    pCam->SetDirHistorySize(HISTORY_WINDOW_OFF);
}

// One, which the broadcast camera clamps its fractions to and starts its shot power scale from.
// GLOBAL: HYPERX 0x004b169c
extern const float g_flCamFracFull;

// Four, added to the look direction's third component before it is normalised.
// GLOBAL: HYPERX 0x004b16a4
extern const float g_flFourTracking;

#define HEAD_BONE 9

// One function that Ghidra splits at 0x0040a2f8.
// FUNCTION: HYPERX 0x0040a100
void CHeadTrack::Process(int nArg)
{
    AgendaItem* pNext;

    auto* const pCam = (CameraObject*)m_pOwner;

    EngineObject* const pPrimary = pCam->GetTrackedObject();

    if (pPrimary == nullptr)
    {
        pNext = m_pNext;

        if (pNext != nullptr)
        {
            pNext->Process(nArg);
        }

        return;
    }

    TransformObject* const pTransform = &pPrimary->m_pBones[HEAD_BONE].m_world;

    pPrimary->GetPhysicsBody();

    m_pWatchPos->x = pTransform->GetElement(0, 3);

    m_pWatchPos->y = pTransform->GetElement(1, 3);

    m_pWatchPos->z = pTransform->GetElement(2, 3);

    EngineObject* const pSecondary = pCam->GetSecondaryTrackedObject();

    if (pSecondary != nullptr)
    {
        short nSlot;
        Vec3  vTarget;
        Vec3  vStep;
        Vec3  vDir;

        vTarget = pSecondary->GetPhysicsBody()->pos;

        nSlot = pSecondary->GetSlot();

        if (nSlot > PLAYER_OBJECT_COUNT)
        {
            m_pWatchPos->z = -10.0f;

            vStep.x = vTarget.x - m_pWatchPos->x;
            vStep.y = vTarget.y - m_pWatchPos->y;
            vStep.z = vTarget.z - m_pWatchPos->z;

            Math::Vec3Normalize(&vStep);

            (&vStep)->ScaleInPlace(100.0f);

            m_pWatchPos->x = vTarget.x - vStep.x;
            m_pWatchPos->y = vTarget.y - vStep.y;
            m_pWatchPos->z = vTarget.z - vStep.z;
        }
        else
        {
            vDir = pPrimary->GetPhysicsBody()->velDir;

            Math::Vec3Normalize(&vDir);

            Vec3::ScaleTo(&vDir, 150.0f, &vStep);

            m_pWatchPos->Add(vStep);

            m_pWatchPos->z += 10.0f;

            Vec3::ScaleTo(&vDir, 20.0f, &vStep);

            vTarget.x -= vStep.x;
            vTarget.y -= vStep.y;
            vTarget.z -= vStep.z;
        }

        m_pWatchDir->x = vTarget.x - m_pWatchPos->x;
        m_pWatchDir->y = vTarget.y - m_pWatchPos->y;
        m_pWatchDir->z = (vTarget.z - m_pWatchPos->z) + g_flFourTracking;

        Math::Vec3Normalize(m_pWatchDir);
    }
    else
    {
        *m_pWatchDir = pPrimary->GetPhysicsBody()->velDir;
    }

    pCam->UpdateSmoothing();

    if (pCam->m_nReticleState != RETICLE_OFF)
    {
        float          flStep;
        AttachPak*     pHolder;
        unsigned short nHeld;
        int            nFollow;

        pHolder = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        if (pHolder != nullptr && pHolder->nSlot < PLAYER_OBJECT_COUNT)
        {
            flStep = g_pGameEngine->m_clock.flDelta;

            PlayerObject* const pCarrier = g_pGameEngine->GetPlayer(pHolder->nSlot);

            flStep = ((pCarrier->m_stats.GetScalar(RATING_SHOT_POWER) - 6.0f) * 0.1f + g_flCamFracFull) * flStep;
        }
        else
        {
            flStep = g_pGameEngine->m_clock.flDelta;
        }

        nFollow = g_pGameEngine->m_apTracked[0]->GetSlot();

        nHeld = (unsigned short)(g_pGameEngine->m_ppSlots[(short)nFollow])->GetPhysicsBody()->nHeldBindings;

        if ((nHeld & HELD_ROTATE) != 0)
        {
            const float flFrac = pCam->m_flViewFracX;

            if ((nHeld & HELD_ROTATE_RIGHT) != 0)
            {
                pCam->SetViewFracX(flFrac + flStep);
            }
            else
            {
                pCam->SetViewFracX(flFrac - flStep);
            }

            if (pCam->m_flViewFracX > g_flCamFracFull)
            {
                pCam->SetViewFracX(g_flCamFracFull);
            }

            if (pCam->m_flViewFracX < g_flZeroTracking)
            {
                pCam->SetViewFracX(g_flZeroTracking);
            }
        }
        else if ((nHeld & HELD_THROTTLE) != 0)
        {
            const float flFrac = pCam->m_flViewFracY;

            if ((nHeld & HELD_FORWARD) != 0)
            {
                pCam->SetViewFracY(flFrac - flStep);
            }
            else
            {
                pCam->SetViewFracY(flFrac + flStep);
            }

            if (pCam->m_flViewFracY > g_flCamFracFull)
            {
                pCam->SetViewFracY(g_flCamFracFull);
            }

            if (pCam->m_flViewFracY < g_flZeroTracking)
            {
                pCam->SetViewFracY(g_flZeroTracking);
            }
        }

        if (g_pRenderer != nullptr)
        {
            ViewExtent size;
            Point      origin;

            size = g_pRenderer->GetHeadCamSize();

            size.ShrinkClamp(g_pRenderer->GetReticleSize());

            origin.Zero();

            double dFracX;
            double dFracY;

            dFracX   = pCam->m_flViewFracX;
            origin.x = (int)(dFracX * size.x);

            dFracY   = pCam->m_flViewFracY;
            origin.y = (int)(dFracY * size.y);

            g_pRenderer->SetReticlePos(origin.x, origin.y);
        }
    }

    pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Process(nArg);
    }
}
