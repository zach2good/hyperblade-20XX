// The chase camera: settling countdown, aim point, spring, plain or smart camera, look direction.

#include "common/compat.h"
#include <algorithm>
#include "math/mathops.h"
#include "planvm/behaviorids.h"
#include "planvm/agendaitemids.h"
#include "planvm/gamebehaviorobject.h"
#include <cstddef>
#include "physics/physicsbody.h"
#include "cameraobject.h"
#include "devicector.h"
#include "objectclasses.h"
#include "planvm/agendaitem.h"
#include "engine/ctrack.h"
#include "physics/collisionellipsoid.h"
#include "engine/objectstats.h"
#include "gameengine.h"
#include "globals.h"
#include "planvm/planvm.h"
#include "common/staticinit.h"
#include "math/vector.h"
#include "planvm/damagegate.h"

#include <cmath>
#include "app/gameapp.h"
#include "playerobject.h"
#include "engine/engineobject.h"
#include "graphics/meshbone.h"
#include "common/pooledownerroot.h"

// Two, the factor an overshooting spring is doubled by.
extern const float g_flCamTrackVelocityGain;

// One, the vertical axis's third component and the arc cosine's first.
extern const float g_flCamFracFull;

// Pi, doubled at each of the two places an arc cosine is reflected into the lower half.
extern const float g_flPi;

// The debug dialog's smart-camera checkbox: set runs the two pulls in sequence.
extern unsigned char g_bSmartCamEnabled;

// The swing is cut into at most six steps however long the countdown is.
#define SWING_STEPS 6

// FUNCTION: HYPERX 0x00408f20
void CTrack::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CTrack;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pPos        = &pOwner->GetPhysicsBody()->pos;
    m_pVel        = &pOwner->GetPhysicsBody()->vel;
    m_pDir        = &pOwner->GetPhysicsBody()->velDir;
    m_pDesiredVel = &pOwner->GetPhysicsBody()->smoothVel;
    m_wCountdown  = 0;

    EngineObject* const pTracked = GetCamera()->GetTrackedObject();

    if (pTracked != nullptr)
    {
        pTracked->m_nFlags |= ENGINEOBJECT_NO_CULL;
    }

    m_bUnused = 0;
}

// FUNCTION: HYPERX 0x00408f90
void CTrack::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pOwner      = nullptr;
        m_pPos        = nullptr;
        m_pVel        = nullptr;
        m_pDir        = nullptr;
        m_pDesiredVel = nullptr;
        m_pNext       = nullptr;
        m_bReady      = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x00408fd0
void CTrack::Process(int nArg)
{
    float flPull;
    Vec3  vWish;
    Vec3  vGap;
    Vec3  aTarget;
    Vec3  vPull;
    Vec3  vSide;
    Vec3  vFinal;
    Vec3  vDir;
    Vec3  vTrack;
    Vec3  vLook;
    Vec3  vFacing;
    Vec3  vRaw;
    Vec3  vSum;
    Vec3  vScaled;
    Vec3  vUp;
    Vec3  vFarRay;
    Vec3  vNearRay;

    const auto flStep = (g_pGameEngine->m_clock.GetDelta() * 50.0f);

    Vec3* const pOffsetScales = GetCamera()->m_pSettings;

    auto* pTracked = (PlayerObject*)GetCamera()->GetTrackedObject();

    if (pTracked == nullptr)
    {
        // STRING: HYPERX 0x004bb454
        g_gameApp.Fatal("CTrack::Process(): Null pointer for model to track\n");
    }

    EngineObject* const pLastTracked = GetCamera()->GetLastTrackedObject();
    PhysicsBody*        pBody        = pTracked->GetPhysicsBody();

    if (pLastTracked == nullptr || pLastTracked != pTracked)
    {
        float flFrames;

        if (pLastTracked != nullptr)
        {
            flFrames = g_pGameEngine->m_clock.flInvDelta * 0.75f;

            if (10.0f > flFrames)
            {
                flFrames = 10.0f;
            }
        }
        else
        {
            flFrames = g_pGameEngine->m_clock.flInvDelta * 1.5f;
        }

        m_wCountdown   = static_cast<short>(flFrames);
        m_bSwingChosen = 0;
        m_bOutOfView   = 1;
    }

    GetCamera()->SetLastTrackedObject(pTracked);

    if (pTracked->GetSlot() < PLAYER_OBJECT_COUNT)
    {
        const TransformObject* const pWorld = &pTracked->m_pBones->m_world;

        aTarget.Set(pWorld->m_a[0][3], pWorld->m_a[1][3], pWorld->m_a[2][3]);

        if ((static_cast<unsigned char>(pBody->nHeldBindings) & HELD_SHOOT) != 0)
        {
            AttachPak* pHolder = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

            if (pHolder != nullptr)
            {
                short wCarrier = pHolder->nSlot;

                if (wCarrier < PLAYER_OBJECT_COUNT && wCarrier == pTracked->GetSlot())
                {
                    EngineObject* const pGoal   = g_pGameEngine->m_ppSlots[pTracked->m_wTargetGoalSlot];
                    const Vec3*         pMine   = pTracked->GetPositionPtr();
                    const Vec3*         pTheirs = pGoal->GetPositionPtr();

                    // Worked out and then never read.
                    vScaled.x = pTheirs->x - pMine->x;
                    vScaled.y = pTheirs->y - pMine->y;
                    vScaled.z = pTheirs->z - pMine->z;

                    Math::Vec3Normalize(&vScaled);
                }
            }
        }
    }
    else
    {
        const short wViewer = g_pGameEngine->m_nViewedSlot;
        AttachPak*  pHolder = pBody->pAttach;

        if (pHolder != nullptr && pHolder->nSlot < PLAYER_OBJECT_COUNT)
        {
            GetCamera()->m_nPrimaryTracked = wViewer;

            GetCamera()->SetSecondaryTrackedObject(nullptr);
        }
        else if (pTracked->GetPlanState()->m_pCurrent->m_wKind != BEH_FLY_TOWARD)
        {
            GetCamera()->m_nPrimaryTracked = wViewer;

            GetCamera()->SetSecondaryTrackedObject(nullptr);
        }

        aTarget = pBody->pos;
    }

    vTrack.x = vLook.x = aTarget.x - m_pPos->x;
    vTrack.y = vLook.y = aTarget.y - m_pPos->y;

    const float flDrop = aTarget.z - m_pPos->z;

    vTrack.z = flDrop + g_flCameraTrackingHeight;
    vLook.z  = flDrop + g_flCameraLookHeight;

    Math::Vec3Normalize(&vLook);

    flPull = (static_cast<float>(sqrt(static_cast<double>(vTrack.x * vTrack.x + (vTrack.y * vTrack.y + vTrack.z * vTrack.z)))) - GetCamera()->GetBungeeLength()) * 1.5f;

    if (flPull <= g_flZeroTracking)
    {
        flPull *= g_flCamTrackVelocityGain;
    }

    Math::Vec3Normalize(&vTrack);

    Vec3::ScaleTo(&vTrack, flPull, m_pDesiredVel);

    const Vec3* pFacing = &pBody->velDir;

    vFacing = *pFacing;

    if (fabs(static_cast<double>(pBody->flFacing)) > 0.1)
    {
        const auto flCos = static_cast<float>(cos(static_cast<double>(pBody->flFacing)));
        const auto flSin = static_cast<float>(sin(static_cast<double>(pBody->flFacing)));

        vRaw.x             = vFacing.z * g_flZeroTracking - vFacing.y * g_flCamFracFull;
        const float flRawY = vFacing.x * g_flCamFracFull - vFacing.z * g_flZeroTracking;

        vFacing.x = vRaw.x * flSin + vFacing.x * flCos;
        vFacing.y = flRawY * flSin + vFacing.y * flCos;
    }

    const short wState = pTracked->GetPlanState()->m_pCurrent->m_wKind;

    if (wState == BEH_FLY || wState == BEH_RAIL_SLIDE || m_wCountdown != 0)
    {
        float flSpeed;

        m_pVel->x = (m_pDesiredVel->x - m_pVel->x * 0.6f) + m_pVel->x;
        m_pVel->y = (m_pDesiredVel->y - m_pVel->y * 0.6f) + m_pVel->y;
        m_pVel->z = (m_pDesiredVel->z - m_pVel->z * 0.6f) + m_pVel->z;

        const Vec3* pVel = m_pVel;

        if ((flSpeed = static_cast<float>(sqrt(static_cast<double>(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z)))) > flStep)
        {
            m_pVel->ScaleInPlace(flStep / flSpeed);
        }

        m_pPos->x = m_pVel->x + m_pPos->x;
        m_pPos->y = m_pVel->y + m_pPos->y;
        m_pPos->z = m_pVel->z + m_pPos->z;

        if ((m_pPos->x * m_pPos->GetX()) * g_camTrackEllipsoid.GetInvXX() + (m_pPos->z * m_pPos->z * g_camTrackEllipsoid.GetInvZZ() + m_pPos->y * m_pPos->y * g_camTrackEllipsoid.GetInvYY()) > 1.0)
        {
            Vec3* pClosest;

            vRaw.x = g_flZeroTracking;
            vRaw.y = g_flZeroTracking;
            vRaw.z = g_flZeroTracking;

            g_camTrackEllipsoid.IntersectSegment(m_pPos, &vRaw, &vFarRay, &vNearRay);

            pClosest = Vec3::FindClosest(m_pPos, &vFarRay, &vNearRay);

            *m_pPos = *pClosest;
        }

        if (m_wCountdown != 0)
        {
            m_bOutOfView = 1;
            m_wCountdown--;
        }
    }
    else
    {
        Vec3 vBack;

        Vec3::ScaleTo(&vFacing, -GetCamera()->GetBungeeLength(), &vBack);

        vWish.x = vBack.x + aTarget.x;
        vWish.y = vBack.y + aTarget.y;
        vWish.z = vBack.z + aTarget.z;

        if (vWish.x * vWish.x * g_camTrackEllipsoid.GetInvXX() + (vWish.y * vWish.y * g_camTrackEllipsoid.GetInvYY() + vWish.z * vWish.z * g_camTrackEllipsoid.GetInvZZ()) > 1.0)
        {
            Vec3  vNearSeg;
            Vec3* pClosest;

            vPull.x = g_flZeroTracking;
            vPull.y = g_flZeroTracking;
            vPull.z = g_flZeroTracking;

            g_camTrackEllipsoid.IntersectSegment(&vWish, &vPull, &vNearSeg, &vRaw);

            pClosest = Vec3::FindClosest(&vWish, &vNearSeg, &vRaw);

            vWish.x = pClosest->x;
            vWish.y = pClosest->y;
            vWish.z = pClosest->z;
        }

        vPull.x = vWish.x - m_pPos->x;
        vPull.y = vWish.y - m_pPos->y;
        vPull.z = vWish.z - m_pPos->z;

        if (g_bSmartCamEnabled != 0)
        {
            float flSpeed;

            m_pVel->x = (m_pDesiredVel->x - m_pVel->x * 0.6f) + m_pVel->x;
            m_pVel->y = (m_pDesiredVel->y - m_pVel->y * 0.6f) + m_pVel->y;
            m_pVel->z = (m_pDesiredVel->z - m_pVel->z * 0.6f) + m_pVel->z;

            const Vec3* pVel = m_pVel;

            if ((flSpeed = static_cast<float>(sqrt(static_cast<double>(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z)))) > flStep)
            {
                m_pVel->ScaleInPlace(flStep / flSpeed);
            }

            m_pVel->x = vPull.x * g_flSolarWindFactor + m_pVel->x;
            m_pVel->y = vPull.y * g_flSolarWindFactor + m_pVel->y;
            m_pVel->z = vPull.z * g_flSolarWindFactor + m_pVel->z;

            pVel = m_pVel;

            flSpeed = static_cast<float>(sqrt(static_cast<double>(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z)));

            if (flSpeed > flStep * 2.5f)
            {
                m_pVel->ScaleInPlace(flStep * 2.5f / flSpeed);
            }
        }
        else
        {
            float flSpeed;

            m_pVel->x = vPull.x * g_flSolarWindFactor + (m_pDesiredVel->x - m_pVel->x * 0.6f) + m_pVel->x;
            m_pVel->y = vPull.y * g_flSolarWindFactor + (m_pDesiredVel->y - m_pVel->y * 0.6f) + m_pVel->y;
            m_pVel->z = vPull.z * g_flSolarWindFactor + (m_pDesiredVel->z - m_pVel->z * 0.6f) + m_pVel->z;

            const Vec3* pVel = m_pVel;

            if ((flSpeed = static_cast<float>(sqrt(static_cast<double>(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z)))) > flStep)
            {
                m_pVel->ScaleInPlace(flStep / flSpeed);
            }
        }

        m_pPos->x = m_pVel->x + m_pPos->x;
        m_pPos->y = m_pVel->y + m_pPos->y;
        m_pPos->z = m_pVel->z + m_pPos->z;

        if (pTracked->GetSlot() < PLAYER_OBJECT_COUNT)
        {
            float  flDeltaX;
            float  flDeltaY;
            float  flAbsX;
            float  flAbsY;
            float  flHypot;
            float  flOffset;
            float* pPos;

            if (pTracked->m_nKnockedDownState != 0)
            {
                m_bOutOfView = 1;
            }

            pPos = &m_pPos->x;

            flDeltaY = vWish.y - aTarget.y;
            flAbsY   = static_cast<float>(fabs(static_cast<double>(flDeltaY)));

            vPull = aTarget;

            flDeltaX = vWish.x - aTarget.x;
            flAbsX   = static_cast<float>(fabs(static_cast<double>(flDeltaX)));

            flHypot = static_cast<float>(sqrt(static_cast<double>(flAbsX * flAbsX + flAbsY * flAbsY)));

            if (flAbsX < 1e-06)
            {
                flOffset = static_cast<float>(fabs(static_cast<double>(pPos[0] - vWish.x)));
            }
            else
            {
                float flSlope;
                float flOnLine;

                flSlope = flDeltaY / flDeltaX;

                flOnLine = (aTarget.y - flSlope * aTarget.x) + flSlope * pPos[0];

                flOffset = static_cast<float>(fabs(static_cast<double>(flOnLine - pPos[1])) * cos(static_cast<double>(Math::Asin(flAbsY / flHypot))));
            }

            if (flOffset > 2.0f)
            {
                if (m_bOutOfView == 0)
                {
                    vGap.x = vWish.x - pPos[0];
                    vGap.y = vWish.y - pPos[1];
                    vGap.z = vWish.z - pPos[2];

                    const Vec3 vHalfGap = vGap.Scaled(0.5f);

                    pPos[0] = vHalfGap.x + pPos[0];
                    pPos[1] = vHalfGap.y + pPos[1];
                    pPos[2] = vHalfGap.z + pPos[2];

                    vRaw = *pFacing;

                    Vec3::ScaleTo(&vRaw, pBody->flSpeed, &vGap);

                    vPull.x = vGap.x + aTarget.x;
                    vPull.y = vGap.y + vPull.y;
                    vPull.z += vGap.z;

                    Vec3::ScaleTo(&vFacing, -GetCamera()->GetBungeeLength(), &vGap);

                    Vec3 vChasePos;

                    vChasePos.x = vGap.x + vPull.x;
                    vChasePos.y = vGap.y + vPull.y;
                    vChasePos.z = vPull.GetZ() + vGap.z;

                    vGap.x = vChasePos.x - m_pPos->x;
                    vGap.y = vChasePos.y - m_pPos->y;
                    vGap.z = vChasePos.z - m_pPos->z;

                    m_pVel->Add(vGap.Scaled(0.25f));
                }
            }
            else
            {
                m_bOutOfView = 0;
            }
        }

        Vec3*       pSumClosest;
        const float flScaleX = pOffsetScales->x;

        vSide.x = g_flZeroTracking;
        vSide.y = g_flZeroTracking;
        vSide.z = g_flZeroTracking;

        vDir.x = g_flZeroTracking;
        vDir.y = g_flZeroTracking;
        vDir.z = g_flZeroTracking;

        vUp.x = g_flZeroTracking;
        vUp.y = g_flZeroTracking;
        vUp.z = g_flCamFracFull;

        if (flScaleX != 0.0f || pOffsetScales->y != 0.0f || pOffsetScales->z != 0.0f)
        {
            vDir = *m_pDir;

            vSide.x = vDir.y * g_flCamFracFull - vDir.z * g_flZeroTracking;
            vSide.y = vDir.z * g_flZeroTracking - vDir.x * g_flCamFracFull;
            vSide.z = vDir.x * g_flZeroTracking - vDir.y * g_flZeroTracking;

            vSide.ScaleInPlace(flScaleX);
            vDir.ScaleInPlace(pOffsetScales->y);
            vUp.ScaleInPlace(pOffsetScales->z);
        }

        vSum.x = vDir.x + vSide.x + m_pPos->x;
        vSum.y = vDir.y + m_pPos->y + vSide.y;
        vSum.z = vUp.z + m_pPos->z + vDir.z + vSide.z;

        Vec3::ScaleTo(&pBody->surfNormal, g_flCameraMinHeight, &vScaled);

        g_camTrackEllipsoid.IntersectRay(&vScaled, &vSum, &vNearRay, &vFarRay);

        pSumClosest = Vec3::FindClosest(&vSum, &vNearRay, &vFarRay);

        vSum = *pSumClosest;

        vFinal.x = aTarget.x - (vScaled.x + vSum.x);
        vFinal.y = aTarget.y - (vScaled.GetY() + vSum.y);
        vFinal.z = aTarget.z - (vScaled.GetZ() + vSum.z);

        Math::Vec3Normalize(&vFinal);

        const double dFollow = GetCamera()->GetBungeeLength();

        vFinal.ScaleInPlace(static_cast<float>(dFollow));

        m_pPos->x = aTarget.x - vFinal.x;
        m_pPos->y = aTarget.y - vFinal.y;
        m_pPos->z = aTarget.z - vFinal.z;
    }

    EngineObject* pSecondary = GetCamera()->GetSecondaryTrackedObject();

    if (pSecondary != nullptr)
    {
        const Vec3* pSecondaryPos;

        if (pSecondary->GetSlot() == SLOT_ROK)
        {
            AttachPak* pHolder = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

            if (pHolder != nullptr && pHolder->nSlot < PLAYER_OBJECT_COUNT)
            {
                pSecondary = g_pGameEngine->m_ppSlots[pHolder->nSlot];
            }
        }

        pSecondaryPos = &pSecondary->GetPhysicsBody()->pos;

        vSide.x = g_flZeroTracking;
        vSide.y = g_flZeroTracking;
        vSide.z = g_flZeroTracking;

        vSide.x = pSecondaryPos->x - m_pPos->x;
        vSide.y = pSecondaryPos->y - m_pPos->y;
        vSide.z = pSecondaryPos->z - m_pPos->z;

        Math::Vec3Normalize(&vSide);

        m_pDir->x = vSide.x + vTrack.x;
        m_pDir->y = vSide.y + vTrack.y;
        m_pDir->z = vSide.z + vTrack.z;

        Math::Vec3Normalize(m_pDir);
    }
    else if (m_wCountdown != 0)
    {
        float flStepAngle;
        float flCount;
        float flDirAngle;
        float flLookAngle;
        float flAngle;
        int   nSteps;

        if (m_bSwingChosen != 0)
        {
            *m_pDir = m_vLastDir;
        }

        vGap   = *m_pDir;
        vGap.z = g_flZeroTracking;

        Math::Vec3Normalize(&vGap);

        vWish   = vLook;
        vWish.z = g_flZeroTracking;

        Math::Vec3Normalize(&vWish);

        flAngle = static_cast<float>(Math::Acos(vWish.x * vGap.x + ((vWish.GetY() * vGap.y) + vWish.GetZ() * vGap.z)));

        nSteps = std::min<int>(SWING_STEPS, m_wCountdown);

        flCount = static_cast<float>(nSteps);

        flStepAngle = flAngle / flCount;

        flDirAngle = static_cast<float>(Math::Acos(vGap.x * g_flCamFracFull + (vGap.GetY() + vGap.z) * g_flZeroTracking));

        if (vGap.y < 0.0f)
        {
            flDirAngle = (g_flPi + g_flPi) - flDirAngle;
        }

        flLookAngle = static_cast<float>(Math::Acos(vWish.x * g_flCamFracFull + (vWish.GetY() + vWish.z) * g_flZeroTracking));

        if (vWish.y < 0.0f)
        {
            flLookAngle = (g_flPi + g_flPi) - flLookAngle;
        }

        if (m_bSwingChosen == 0)
        {
            if (flDirAngle > flLookAngle)
            {
                flAngle = flDirAngle - flStepAngle;

                m_bSwingBack = 1;
            }
            else
            {
                flAngle = flDirAngle + flStepAngle;

                m_bSwingBack = 0;
            }

            m_bSwingChosen = 1;
        }
        else if (flDirAngle > flLookAngle)
        {
            flAngle = flDirAngle - flStepAngle;
        }
        else
        {
            flAngle = flDirAngle + flStepAngle;
        }

        vFinal.x = static_cast<float>(cos(static_cast<double>(flAngle)));
        vFinal.y = static_cast<float>(sin(static_cast<double>(flAngle)));
        vFinal.z = g_flZeroTracking;

        Math::Vec3Normalize(&vFinal);

        vGap = *m_pDir;

        Math::Vec3Normalize(&vGap);

        vWish = vLook;

        Math::Vec3Normalize(&vWish);

        vFinal.z = (vWish.z - vGap.z) / flCount + vGap.z;

        *m_pDir = vFinal;

        Math::Vec3Normalize(m_pDir);

        m_vLastDir = *m_pDir;
    }
    else
    {
        *m_pDir = vLook;
    }

    GetCamera()->UpdateSmoothing();

    if (m_pNext != nullptr)
    {
        m_pNext->Process(nArg);
    }
}
