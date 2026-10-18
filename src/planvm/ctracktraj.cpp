#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "planvm/agendaitem.h"
#include "common/pooledclasses.h"
#include "physics/physicsbody.h"
#include "engine/cameraobject.h"
#include "engine/devicector.h"
#include "physics/collisionellipsoid.h"
#include "engine/objectstats.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "engine/engineglobals.h"
#include "common/staticinit.h"
#include "math/vector.h"
#include "damagegate.h"
#include "math/spline.h"
#include "planvm/planvmglobals.h"
#include "app/gameapp.h"
#include <new>
#include <cmath>
#include "engine/engineobject.h"
#include "planvm/ctracktraj.h"

// FUNCTION: HYPERX 0x0040a510
CTrackTraj::CTrackTraj()
{
    m_pSpline = new Spline;
}

// Two: the share of its own velocity the camera spends on position in each axis.
// GLOBAL: HYPERX 0x004b16a0
extern const float g_flCamTrackVelocityGain;

// The debug dialog's checkbox 0x4b8; while clear, the blended mode keeps its look direction.
// GLOBAL: HYPERX 0x004bc3d4
extern char g_bCamTrackLookEnabled;

// Four, the clearance the camera keeps above the arena floor; marked in cheadtrack.cpp.
extern const float g_flFourTracking;

#define TRAJECTORY_MODE_SPRING 0
#define TRAJECTORY_MODE_BLEND  1

// FUNCTION: HYPERX 0x0040a5c0
CTrackTraj::~CTrackTraj()
{
    Spline* const pList = m_pSpline;

    if (pList != nullptr)
    {
        delete pList;

        m_pSpline = nullptr;
    }
}

// FUNCTION: HYPERX 0x0040a630
void CTrackTraj::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    EngineObject* pTracked;

    m_pNext  = nullptr;
    m_wKind  = Agenda_CTrackTraj;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_vTrajPos = pOwner->GetPhysicsBody()->pos;

    pTracked = GetCamera()->GetTrackedObject();

    if (pTracked != nullptr)
    {
        pTracked->m_nFlags |= ENGINEOBJECT_NO_CULL;
    }
}

// The high bound adds the frame-rate reference, eighty, which is not a distance here.

// FUNCTION: HYPERX 0x0040a680
void CTrackTraj::InitRange()
{
    const double dLow   = g_arenaEllipsoid.GetRadiusX();
    float        flHigh = g_arenaEllipsoid.GetRadiusY();
    flHigh += g_flFrameRateReference;

    m_pSpline->InitClamped(static_cast<float>(dLow), flHigh);

    m_nMode   = 0;
    m_flBlend = g_flZeroTracking;
}

#define RANGE_KIND_CLAMPED   0
#define RANGE_KIND_SYMMETRIC 2

// A low bound at or below the unset marker builds a symmetric range of kind two instead.
// FUNCTION: HYPERX 0x0040a6c0
void CTrackTraj::SetDirectedRange(float flLow, float flHigh)
{
    if (flLow > 0.0f)
    {
        m_pSpline->InitClamped(flLow, flHigh);

        m_nMode   = RANGE_KIND_CLAMPED;
        m_flBlend = g_flZeroTracking;

        return;
    }

    m_pSpline->InitSymmetric(g_arenaEllipsoid.GetRadiusX() - flHigh,
                             g_arenaEllipsoid.GetRadiusY() - flHigh);

    m_nMode = RANGE_KIND_SYMMETRIC;
}

// GLOBAL: HYPERX 0x004b16ac
extern const float g_flPi;

// FUNCTION: HYPERX 0x0040a730
void CTrackTraj::InitComplexRange()
{
    CollisionEllipsoid* const pArena = &g_arenaEllipsoid;

    // Through its getter, the radius loads before pi, as the client loads it.
    m_pSpline->InitSpringNode(-(pArena->GetRadiusX() * g_flCamWatchMidpointScale),
                              g_flPi / pArena->GetRadiusX(),
                              pArena->GetRadiusX() * -0.4f,
                              g_flPi * 0.1f);

    m_nMode   = 1;
    m_flBlend = g_flZeroTracking;
}

// A negative fraction widens as unsigned and becomes a very large one.

// FUNCTION: HYPERX 0x0040a790
void CTrackTraj::SetBlendT(int nValue)
{
    m_flBlend = static_cast<float>(static_cast<unsigned int>(nValue));
}

// Mode zero chases the tracked object on a spring; mode one blends toward a scripted point.
// FUNCTION: HYPERX 0x0040a7b0
void CTrackTraj::Process(int nArg)
{
    Vec3* const pCamPos = &m_pOwner->GetPhysicsBody()->pos;
    Vec3* const pCamVel = GetCamera()->m_pSettings;
    Vec3* const pCamDir = &m_pOwner->GetPhysicsBody()->velDir;

    auto* const pCamera = (CameraObject*)m_pOwner;

    if (pCamera->GetTrackedObject() == nullptr)
    {
        EngineObject* pViewer;

        pViewer = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];

        if (pViewer == nullptr)
        {
            // STRING: HYPERX 0x004bb4c0
            g_gameApp.Fatal("CTrackTraj::Process(): Null pointer for view owner\n");
        }

        pCamera->SetTrackedObject(pViewer);
    }

    Vec3* const pTracked = &GetCamera()->GetTrackedObject()->GetPhysicsBody()->pos;

    EngineObject* const pWatched = GetCamera()->GetSecondaryTrackedObject();

    if (pWatched == nullptr)
    {
        // STRING: HYPERX 0x004bb488
        g_gameApp.Fatal("CTrackTraj::Process(): Null pointer for model to watch\n");
    }

    Vec3 vWatch = *pWatched->GetPositionPtr();

    switch (m_nMode)
    {
        case TRAJECTORY_MODE_SPRING:
        {
            Vec3* pAccel;
            float flWatchX;
            float flWatchY;
            float flWatchZ;
            Vec3  vMidOffset;
            Vec3  vTrajOffset;
            Vec3  vMidPull;
            Vec3  vTrajPull;
            Vec3  vScratch;
            float flRadius;
            float flMidLength;
            float flTrajLength;
            float flFloor;

            m_pSpline->UpdateTowardTarget(&m_vTrajPos, pTracked);

            pAccel = &m_pOwner->GetPhysicsBody()->vel;

            flWatchX = vWatch.x - pTracked->x;
            flWatchY = vWatch.y - pTracked->y;
            flWatchZ = vWatch.z - pTracked->z;

            // The midpoint and the cross product below share one scratch vector.
            vScratch.x = g_flCamWatchMidpointScale * flWatchX + pTracked->x;
            vScratch.y = g_flCamWatchMidpointScale * flWatchY + pTracked->y;
            vScratch.z = g_flCamWatchMidpointScale * flWatchZ + pTracked->z;

            vTrajOffset.x = m_vTrajPos.x - pCamPos->x;
            vTrajOffset.y = m_vTrajPos.y - pCamPos->y;
            vTrajOffset.z = m_vTrajPos.z - pCamPos->z;

            vMidOffset.x = vScratch.x - pCamPos->x;
            vMidOffset.y = vScratch.y - pCamPos->y;
            vMidOffset.z = vScratch.z - pCamPos->z;

            vScratch.x = flWatchZ * pCamDir->y - flWatchY * pCamDir->z;
            vScratch.y = flWatchX * pCamDir->z - flWatchZ * pCamDir->x;
            vScratch.z = flWatchY * pCamDir->x - flWatchX * pCamDir->y;

            flRadius = static_cast<float>(sqrt(static_cast<double>(vScratch.x * vScratch.x + (vScratch.y * vScratch.y + vScratch.z * vScratch.z)))) * 1.2f;

            if (flRadius > 190.0)
            {
                flRadius = 190.0f;
            }

            flMidLength = static_cast<float>(sqrt(static_cast<double>(vMidOffset.x * vMidOffset.x + (vMidOffset.y * vMidOffset.y + vMidOffset.z * vMidOffset.z))));

            flTrajLength = static_cast<float>(sqrt(static_cast<double>(vTrajOffset.x * vTrajOffset.x + (vTrajOffset.y * vTrajOffset.y + vTrajOffset.z * vTrajOffset.z))));

            Math::Vec3Normalize(&vMidOffset);
            Math::Vec3Normalize(&vTrajOffset);

            Vec3::ScaleTo(&vMidOffset, (flMidLength - flRadius) * 0.6f, &vMidPull);
            Vec3::ScaleTo(&vTrajOffset, flTrajLength * 0.8f, &vTrajPull);

            pAccel->x = (((vTrajPull.x + vMidPull.x) - pAccel->x * 0.6f) - g_flCamWatchMidpointScale * pCamPos->x) + pAccel->x;
            pAccel->y = (((vTrajPull.y + vMidPull.y) - pAccel->y * 0.6f) - g_flCamWatchMidpointScale * pCamPos->y) + pAccel->y;
            pAccel->z = (((vTrajPull.z + vMidPull.z) - pAccel->z * 0.6f) - (g_arenaEllipsoid.GetRadiusX() + pCamPos->z) * g_flCamWatchMidpointScale) + pAccel->z;

            // The first sum names the position first; the original loads it first.
            pCamPos->x += pAccel->x;
            pCamPos->y = pAccel->y + pCamPos->y;
            pCamPos->z = pAccel->z + pCamPos->z;

            if ((flFloor = g_arenaEllipsoid.GetZ(pCamPos->x, pCamPos->y)) > pCamPos->z)
            {
                pCamPos->z = flFloor + g_flFourTracking;
            }

            pCamDir->x = vWatch.x - pCamPos->x;
            pCamDir->y = vWatch.y - pCamPos->y;
            pCamDir->z = vWatch.z - pCamPos->z;

            Math::Vec3Normalize(pCamDir);

            pCamPos->z = pCamVel->y + pCamPos->z;
        }

        break;

        case TRAJECTORY_MODE_BLEND:
            if (m_flBlend < 9000.0)
            {
                Vec3* pAccel;
                Vec3  vRest;
                float flPosY;
                float flPosZ;
                Vec3  vMid;
                Vec3  vOffset;
                Vec3  vPull;

                vRest.x = 104.0f;
                vRest.y = g_flZeroTracking;
                vRest.z = g_flZeroTracking;

                pAccel = &m_pOwner->GetPhysicsBody()->vel;

                vMid.x = (vWatch.x + pTracked->x) * g_flCamWatchMidpointScale;
                vMid.y = (vWatch.y + pTracked->y) * g_flCamWatchMidpointScale;
                vMid.z = (vWatch.z + pTracked->z) * g_flCamWatchMidpointScale;

                vOffset.x = vMid.x - pCamPos->x;
                vOffset.y = vMid.y - pCamPos->y;
                vOffset.z = vMid.z - pCamPos->z;

                pCamPos->x = vOffset.x * 0.2f + pCamPos->x;
                flPosY     = vOffset.y * 0.2f + pCamPos->y;
                pCamPos->y = flPosY;

                flPosZ     = vOffset.z * 0.2f + pCamPos->z;
                pCamPos->z = flPosZ;

                vPull.x = (vRest.x - pCamPos->x) * 0.1f;
                vPull.y = (vRest.y - flPosY) * 0.1f;
                vPull.z = (vRest.z - flPosZ) * 0.1f;

                pAccel->x = (vPull.x - pAccel->x * 0.6f) + pAccel->x;
                pAccel->y = (vPull.y - pAccel->y * 0.6f) + pAccel->y;
                pAccel->z = (vPull.z - pAccel->z * 0.6f) + pAccel->z;

                pCamPos->x = pAccel->x + pCamPos->x;
                pCamPos->y = pAccel->y + pCamPos->y;
                pCamPos->z = pAccel->z + pCamPos->z;
            }
            else if (m_flBlend > 10000.0)
            {
                Vec3* pVel;

                pVel = GetCamera()->m_pSettings;

                if (pVel->x != g_flZeroTracking)
                {
                    pCamPos->x = g_flCamTrackVelocityGain * pVel->x + pCamPos->x;

                    GetCamera()->m_pSettings->x = g_flZeroTracking;
                }

                pVel = GetCamera()->m_pSettings;

                if (pVel->y != g_flZeroTracking)
                {
                    pCamPos->y = g_flCamTrackVelocityGain * pVel->y + pCamPos->y;

                    GetCamera()->m_pSettings->y = g_flZeroTracking;
                }

                pVel = GetCamera()->m_pSettings;

                if (pVel->z != g_flZeroTracking)
                {
                    pCamPos->z = g_flCamTrackVelocityGain * pVel->z + pCamPos->z;

                    GetCamera()->m_pSettings->z = g_flZeroTracking;
                }

                if (g_bCamTrackLookEnabled == 0)
                {
                    break;
                }
            }

            pCamDir->x = vWatch.x - pCamPos->x;
            pCamDir->y = vWatch.y - pCamPos->y;
            pCamDir->z = vWatch.z - pCamPos->z;

            Math::Vec3Normalize(pCamDir);

            break;
    }

    GetCamera()->UpdateSmoothing();

    if (m_pNext != nullptr)
    {
        m_pNext->Process(nArg);
    }
}
