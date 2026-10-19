#include "common/compat.h"
#include "engine/messageids.h"
#include "planvm/ctracktraj.h"
#include "planvm/csteertotarget.h"
#include "planvm/csteertoward.h"
#include "engine/devicector.h"
#include "anim/animbinding.h"
#include "anim/animset.h"
#include "engine/objectclasses.h"
#include "engine/weaponobject.h"
#include "plannode.h"
#include "globals.h"
#include "planvm.h"
#include "common/pooledclasses.h"
#include "anim/animdb.h"
#include "anim/animregistry.h"
#include "engine/gameengine.h"
#include "engine/cameraobject.h"
#include "physics/physicsbody.h"
#include "math/vector.h"
#include "planvmglobals.h"
#include "ui/hudglobals.h"
#include "engine/particle.h"
#include "planvm/agendaitem.h"
#include "planvm/cbungeetoward.h"
#include "engine/cspeedburst.h"
#include "engine/csetpositionrelative.h"
#include "planvm/cpaletteeffect.h"
#include "graphics/meshbone.h"
#include "math/transformobject.h"
#include "physics/upaxis.h"
#include "engine/objectstats.h"
#include "damagegate.h"
#include "physics/boundbubble.h"
#include "graphics/meshboundingbox.h"
#include "engine/teamblock.h"
#include "common/stub.h"
#include "planarg.h"
#include "engine/messagequeue.h"
#include "physics/collisionellipsoid.h"
#include "graphics/colorpalette.h"
#include "graphics/meshmaterial.h"
#include "graphics/modeldatabase.h"
#include "engine/rampobject.h"
#include "axisframe.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "input/keyboarddeviceobj.h"
#include "engine/playerobject.h"
#include "anim/animplayer.h"
#include "app/gameapp.h"
#include "engine/rokobject.h"
#include "ui/infobar.h"
#include "physics/collisionobj.h"
#include <cmath>
#include <cstdlib>
#include "engine/particlesystem.h"
#include "physics/collisionplane.h"
#include "engine/engineobject.h"
#include "physics/collisioncheck.h"
#include "engine/object.h"
#include "planvm/planaction.h"
#include "planvm/plancond.h"
#include "common/staticinit.h"

extern const float g_flZeroPlanTracking    = 0.0f;
float              g_flUnusedPlanTrackingA = g_flZeroPlanTracking;
float              g_flUnusedPlanTrackingB = g_flZeroPlanTracking;
float              g_flUnusedPlanTrackingC = g_flZeroPlanTracking;

// Its marker is in src/common/behobjmonitorkeyboard.cpp.
extern int g_bTwoPlayers;

// With two players, the slot's low bit picks which joystick half of the tracked pair is written.
// FUNCTION: HYPERX 0x00401820
void PlanAction::SetObjectToTrack(Interactor* pContext, PlanNode* pArgs, short*, int nSlot)
{
    short nValue;
    int   nJoystick;

    auto* const pCamera = g_pGameEngine->GetCamera(kCameraMainSlot);

    if (pCamera != nullptr)
    {
        const PlanWord* const pBlock = pArgs->m_pArgs;
        if (pBlock[1].IsPronoun())
        {
            const auto pfnResolver = g_apPlanStatFns[pBlock[1].wPronoun];

            nValue = pfnResolver(pContext->GetActor(), pArgs);
        }
        else if (pBlock[1].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nValue = static_cast<short>(nSlot);
        }
        else
        {
            nValue = static_cast<short>(pBlock[1].n);
        }

        const int nIndex = nValue;

        pCamera->m_nPrimaryTracked = nIndex;

        if (g_bTwoPlayers != 0)
        {
            g_pGameEngine->m_nViewedSlot = nValue;

            if ((nValue & 1) != 0)
            {
                nJoystick = g_nJoystickSlotB;
            }
            else
            {
                nJoystick = g_nJoystickSlotA;
            }

            g_pGameEngine->m_apTracked[nJoystick] = g_pGameEngine->m_ppSlots[nIndex];

            return;
        }

        if (nValue < kPlayerObjectCount)
        {
            g_pGameEngine->m_nViewedSlot = nValue;

            g_pGameEngine->m_apTracked[0] = g_pGameEngine->m_ppSlots[nIndex];
            g_pGameEngine->m_apTracked[1] = g_pGameEngine->m_ppSlots[nIndex];
        }
    }
}

// A camera tracking the Rok moves to the target only while the Rok is attached to it.

// FUNCTION: HYPERX 0x00401900
void PlanAction::IfNotRokTrackMe(Interactor* pContext, PlanNode*, short*, int)
{
    CameraObject* pObject;
    AttachPak*    pTracked;

    const int nTarget = pContext->GetTargetSlot();

    if (pContext != nullptr)
    {
        pObject = pContext->GetCameraActor();
    }
    else
    {
        pObject = g_pGameEngine->GetCamera(kCameraMainSlot);
    }

    if (pObject->m_nPrimaryTracked == kSlotRok)
    {
        pTracked = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

        if (pTracked == nullptr)
        {
            return;
        }

        if (pTracked->nSlot != nTarget)
        {
            return;
        }
    }

    pObject->m_nPrimaryTracked = nTarget;
}

// FUNCTION: HYPERX 0x00401960
void PlanAction::SetTrackToViewOwner(Interactor* pContext, PlanNode*, short*, int)
{
    CameraObject* pObject;

    if (pContext != nullptr)
    {
        pObject = pContext->GetCameraActor();
    }
    else
    {
        pObject = g_pGameEngine->GetCamera(kCameraMainSlot);
    }

    // The actor's class here is not established; the other arm takes slot nine, a camera.
    pObject->m_nPrimaryTracked = g_pGameEngine->m_nViewedSlot;
}

// A count of two copies a marker's floats; otherwise the arguments are whole units.

// FUNCTION: HYPERX 0x004019a0
void PlanAction::SetTrackingOffset(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    CameraObject* pActor;

    if (pArgs->m_nArgCount == 2)
    {
        auto* const pMarker = g_apTrackOffsets[pArgs->m_pArgs[1].n];

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->x = pMarker->x;

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->y = pMarker->y;

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->z = pMarker->z;

        return;
    }

    pActor                 = pContext->GetCameraActor();
    pActor->m_pSettings->x = static_cast<float>(pArgs->m_pArgs[1].n);

    pActor                 = pContext->GetCameraActor();
    pActor->m_pSettings->y = static_cast<float>(pArgs->m_pArgs[2].n);

    pActor                 = pContext->GetCameraActor();
    pActor->m_pSettings->z = static_cast<float>(pArgs->m_pArgs[3].n);
}

// FUNCTION: HYPERX 0x00401a40
void PlanAction::IncrementTrackingOffset(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    CameraObject* pActor;

    if (pArgs->m_nArgCount == 2)
    {
        auto* const pMarker = g_apTrackOffsets[pArgs->m_pArgs[1].n];

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->x = pMarker->x + pActor->m_pSettings->x;

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->y = pMarker->y + pActor->m_pSettings->y;

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->z = pMarker->z + pActor->m_pSettings->z;

        return;
    }

    // The literals are tenths here where the setter takes whole units.
    pActor = pContext->GetCameraActor();
    pActor->m_pSettings->x =
        static_cast<float>(pArgs->m_pArgs[1].n * 0.1 + pActor->m_pSettings->x);

    pActor = pContext->GetCameraActor();
    pActor->m_pSettings->y =
        static_cast<float>(pArgs->m_pArgs[2].n * 0.1 + pActor->m_pSettings->y);

    pActor = pContext->GetCameraActor();
    pActor->m_pSettings->z =
        static_cast<float>(pArgs->m_pArgs[3].n * 0.1 + pActor->m_pSettings->z);
}

// FUNCTION: HYPERX 0x00401b10
void PlanAction::IncrementCameraDir(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    if (pArgs->m_nArgCount == 2)
    {
        const Vec3* const pMarker = g_apTrackOffsets[pArgs->m_pArgs[1].n];

        pContext->GetActor()->GetPhysicsBody()->velDir.x += pMarker->x;
        pContext->GetActor()->GetPhysicsBody()->velDir.y += pMarker->y;
        pContext->GetActor()->GetPhysicsBody()->velDir.z += pMarker->z;
    }
    else
    {
        // Hundredths here, where the position adder takes tenths.
        pContext->GetActor()->GetPhysicsBody()->velDir.x += static_cast<float>(pArgs->m_pArgs[1].n * 0.01);
        pContext->GetActor()->GetPhysicsBody()->velDir.y += static_cast<float>(pArgs->m_pArgs[2].n * 0.01);

        pContext->GetActor()->GetPhysicsBody()->velDir.z += static_cast<float>(pArgs->m_pArgs[3].n * 0.01);
    }

    auto* const pBody = pContext->GetActor()->GetPhysicsBody();

    Math::Vec3Normalize(&pBody->velDir);
}

// FUNCTION: HYPERX 0x00401c00
void PlanAction::IncrementBungeeLength(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    float flValue;

    auto* pActor = pContext->GetCameraActor();

    flValue = static_cast<float>(pArgs->m_pArgs[1].n * 0.1) + pActor->GetBungeeLength();

    pActor = pContext->GetCameraActor();

    pActor->m_flBungeeLength = flValue;
}

// FUNCTION: HYPERX 0x00401c40
void PlanAction::SetBungeeLength(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    const auto flValue = static_cast<float>(pArgs->m_pArgs[1].n * 0.1);

    auto* const pActor = pContext->GetCameraActor();

    pActor->m_flBungeeLength = flValue;
}

// Plan operations that write one setting each; the three camera heights and the buffer factor take tenths.
// FUNCTION: HYPERX 0x00401c70
void PlanAction::SetTrackHeight(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flCameraTrackingHeight = static_cast<float>(pArgs->m_pArgs[1].n * 0.1);
}

// FUNCTION: HYPERX 0x00401c90
void PlanAction::SetMinHeight(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flCameraMinHeight = static_cast<float>(pArgs->m_pArgs[1].n * 0.1);
}

// FUNCTION: HYPERX 0x00401cb0
void PlanAction::SetLookHeight(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flCameraLookHeight = static_cast<float>(pArgs->m_pArgs[1].n * 0.1);
}

// FUNCTION: HYPERX 0x00401cd0
void PlanAction::SetCamBufferFactor(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flSteadyCamBuffer = static_cast<float>(pArgs->m_pArgs[1].n * 0.1);
}

// FUNCTION: HYPERX 0x00401cf0
void PlanAction::SetScanModes(Interactor*, PlanNode* pArgs, short*, int)
{
    g_bScanningModesEnabled = pArgs->m_pArgs[1].n != 0;
}

// FUNCTION: HYPERX 0x00401d10
void PlanAction::SetSolarWindFactor(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flSolarWindFactor = pArgs->m_pArgs[1].fl;
}

// FUNCTION: HYPERX 0x00401d30
void PlanAction::SetOffsetValues(Interactor*, PlanNode* pArgs, short*, int)
{
    auto* const pPos = g_apTrackOffsets[pArgs->m_pArgs[1].n];

    pPos->x = static_cast<float>(pArgs->m_pArgs[2].n * 0.1);
    pPos->y = static_cast<float>(pArgs->m_pArgs[3].n * 0.1);
    pPos->z = static_cast<float>(pArgs->m_pArgs[4].n * 0.1);
}

constexpr auto kTrackedSlotLimit = 8;

// Every rejection writes -1, so an invalid request clears the tracked object.
// FUNCTION: HYPERX 0x00401d70
void PlanAction::SetObjectToWatch(Interactor* pContext, PlanNode* pArgs, short*, int nSlot)
{
    CameraObject* pTarget;
    int           nValue;

    if (pContext != nullptr)
    {
        pTarget = pContext->GetCameraActor();
    }
    else
    {
        pTarget = g_pGameEngine->GetCamera(kCameraMainSlot);
    }

    const PlanWord* const pBlock = pArgs->m_pArgs;
    if (pBlock[1].IsPronoun())
    {
        const auto pfnResolver = g_apPlanStatFns[pBlock[1].wPronoun];

        nValue = pfnResolver(pContext->GetActor(), pArgs);
    }
    else if (pBlock[1].n == PLAN_TOKEN_STRAIGHTAHEAD)
    {
        nValue = -1;
    }
    else if (pBlock[1].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nValue = static_cast<short>(pBlock[1].n);
    }
    else
    {
        nValue = nSlot;
    }

    if (nValue == -1)
    {
        pTarget->m_nSecondaryTracked = -1;

        return;
    }

    if (static_cast<unsigned int>(nValue) > kTrackedSlotLimit)
    {
        pTarget->m_nSecondaryTracked = -1;

        return;
    }

    if (static_cast<short>(nValue) > g_pGameEngine->m_nLoadedCount)
    {
        pTarget->m_nSecondaryTracked = -1;

        return;
    }

    if (g_pGameEngine->m_ppSlots[static_cast<short>(nValue)] == nullptr)
    {
        pTarget->m_nSecondaryTracked = -1;

        return;
    }

    pTarget->m_nSecondaryTracked = nValue;
}

// The wrap is one short of the object count, so the last entry is never selected.
// FUNCTION: HYPERX 0x00401e40
void PlanAction::IncrementObjectToWatch(Interactor*, PlanNode*, short*, int)
{
    g_nSecondaryCamCursorA++;

    if (g_nSecondaryCamCursorA >= g_pGameEngine->m_nObjectCount - 1)
    {
        g_nSecondaryCamCursorA = 0;
    }

    g_pGameEngine->GetCamera(kCameraMainSlot)->SetSecondaryTrackedObject(g_pGameEngine->m_ppObjects[g_nSecondaryCamCursorA]);
}

// FUNCTION: HYPERX 0x00401e90
void PlanAction::IncrementObjectToTrack(Interactor*, PlanNode*, short*, int)
{
    g_nSecondaryCamCursorB++;

    if (g_nSecondaryCamCursorB >= g_pGameEngine->m_nObjectCount - 1)
    {
        g_nSecondaryCamCursorB = 0;
    }

    g_pGameEngine->GetCamera(kCameraMainSlot)->SetSecondaryTrackedObject(g_pGameEngine->m_ppObjects[g_nSecondaryCamCursorB]);
}

#define POS_WINDOW_SECONDS 0.5
#define DIR_WINDOW_SECONDS 0.4

// Sizes the smoothing windows from the frame rate; at the default factor the position one saturates above about 55 fps.
// FUNCTION: HYPERX 0x00401ee0
void PlanAction::ResetCameraBufferSize(Interactor*, PlanNode*, short*, int)
{
    auto* pEngine = g_pGameEngine.get();

    auto flRate   = pEngine->m_clock.flInvDelta;
    auto flFactor = g_flSteadyCamBuffer;

    auto nPos = static_cast<short>(flRate * flFactor * POS_WINDOW_SECONDS);

    if (nPos > kCameraHistoryCapacity)
    {
        nPos = kCameraHistoryCapacity;
    }
    else if (nPos < 1)
    {
        nPos = 1;
    }

    pEngine->GetCamera(kCameraMainSlot)->SetPosHistorySize(nPos);

    pEngine = g_pGameEngine.get();

    flRate   = pEngine->m_clock.flInvDelta;
    flFactor = g_flSteadyCamBuffer;

    auto nDir = static_cast<short>(flRate * flFactor * DIR_WINDOW_SECONDS);

    if (nDir > kCameraHistoryCapacity)
    {
        nDir = kCameraHistoryCapacity;
    }
    else if (nDir < 1)
    {
        nDir = 1;
    }

    pEngine->GetCamera(kCameraMainSlot)->SetDirHistorySize(nDir);
}

constexpr auto kArgCountOwnCamera   = 1;
constexpr auto kArgCountSlotAndMode = 3;

#define ARG_SLOT 1
constexpr auto kArgMode = 2;

// A half; its marker is in src/globals.cpp.
extern const float g_flViewFracHalf;

// FUNCTION: HYPERX 0x00401f80
void PlanAction::ResetReticle(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    CameraObject* pCamera;
    float         flFracX;
    float         flFracY;

    const auto nArgCount = pArgs->m_nArgCount;

    if (nArgCount == kArgCountOwnCamera)
    {
        flFracX = g_flViewFracHalf;
        pCamera = (CameraObject*)pContext->GetActor();
        pCamera->SetViewFracX(flFracX);

        flFracY = g_flViewFracHalf;
        pCamera = (CameraObject*)pContext->GetActor();
        pCamera->SetViewFracY(flFracY);

        ((CameraObject*)pContext->GetActor())->SetReticleState(kReticleAim);
        return;
    }

    if (nArgCount == kArgCountSlotAndMode)
    {
        auto* const pBlock = pArgs->m_pArgs;

        g_pGameEngine->GetCamera(static_cast<short>(pBlock[ARG_SLOT].n))->SetReticleState(pBlock[kArgMode].n);

        if (pArgs->m_pArgs[kArgMode].n == kReticleOff)
        {
            flFracX = g_flViewFracHalf;
            pCamera = (CameraObject*)pContext->GetActor();
            pCamera->SetViewFracX(flFracX);

            flFracY = g_flViewFracHalf;
            pCamera = (CameraObject*)pContext->GetActor();
            pCamera->SetViewFracY(flFracY);
        }
    }
}

// The renderer hears only if there is text; the reticle goes off either way.

// FUNCTION: HYPERX 0x00402040
void PlanAction::SetHeadCamText(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    if (g_pRenderer != nullptr)
    {
        g_pRenderer->SetHeadCamText(pArgs->m_pArgs[1].psz);
    }

    ((CameraObject*)pContext->GetActor())->SetReticleState(kReticleOff);
}

// Leads the tracked object by three frames of its velocity.

// FUNCTION: HYPERX 0x00402070
void PlanAction::InitRearViewPos(Interactor* pContext, PlanNode*, short*, int)
{
    auto* const pTarget = &pContext->GetActor()->GetPhysicsBody()->pos;

    auto* const pTracked = pContext->GetCameraActor()->GetTrackedObject();

    if (pTracked != nullptr)
    {
        auto* pState = pTracked->GetPhysicsBody();
        auto* pPos   = pTracked->GetPositionPtr();
        pTarget->x   = g_flRearViewLeadFrames * pState->vel.x + pPos->x;

        pState     = pTracked->GetPhysicsBody();
        pPos       = pTracked->GetPositionPtr();
        pTarget->y = g_flRearViewLeadFrames * pState->vel.y + pPos->y;

        pState     = pTracked->GetPhysicsBody();
        pPos       = pTracked->GetPositionPtr();
        pTarget->z = g_flRearViewLeadFrames * pState->vel.z + pPos->z;
    }
}

// FUNCTION: HYPERX 0x00402100
void PlanAction::SetCamOffsetToLocation(Interactor*, PlanNode*, short*, int)
{
    auto* const pCamera = g_pGameEngine->GetCamera(kCameraMainSlot);

    const Vec3* const pPos      = &pCamera->GetPhysicsBody()->pos;
    auto* const       pSettings = pCamera->m_pSettings;

    pSettings->x = pPos->x;
    pSettings->y = pPos->y;
    pSettings->z = pPos->z;
}

// Scales the aim by the frame's delta so it survives one tick of integration.

// FUNCTION: HYPERX 0x00402160
void PlanAction::RelaxYawToFace(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    Vec3          vAim;
    EngineObject* pTarget;
    float         flDelta;

    pTarget                = g_pGameEngine->m_ppSlots[static_cast<short>(pArgs->m_pArgs[1].n)];
    auto* const pTargetPos = pTarget->GetPositionPtr();

    auto*       pActor   = pContext->GetActor();
    auto* const pSelfPos = pActor->GetPositionPtr();

    pActor              = pContext->GetActor();
    auto* const pFacing = pActor->GetPhysicsBody();

    pActor             = pContext->GetActor();
    auto* const pBlock = pActor->GetPhysicsBody();

    auto* const pVelocity = &pBlock->vel;

    vAim.x = pTargetPos->x - pSelfPos->x;
    vAim.y = pTargetPos->y - pSelfPos->y;
    vAim.z = pTargetPos->z - pSelfPos->z;

    Math::Vec3Normalize(&vAim);

    *pVelocity      = vAim;
    pFacing->velDir = pBlock->GetVel();

    flDelta = g_pGameEngine->m_clock.flDelta;

    pVelocity->ScaleInPlace(flDelta);
}

constexpr auto kArgSpread = 1;
constexpr auto kArgSpeed  = 2;

// The defaults, taken whenever the block does not carry the number.
#define DEFAULT_SPREAD 20.0f
#define DEFAULT_SPEED  50.0f

// FUNCTION: HYPERX 0x00402230
void PlanAction::ExplodeFlat(Interactor* pContext, PlanNode* pNode, short*, int)
{
    float flSpread;
    float flSpeed;

    auto* const pEntity = g_pGameEngine->GetParticleSystem(kSlotExplosion);

    const auto nArgCount = pNode->m_nArgCount;

    if (nArgCount < 2)
    {
        flSpread = DEFAULT_SPREAD;
        flSpeed  = DEFAULT_SPEED;
    }
    else if (nArgCount < 3)
    {
        flSpread = static_cast<float>(pNode->m_pArgs[kArgSpread].n);
        flSpeed  = DEFAULT_SPEED;
    }
    else
    {
        auto* const pArgs = pNode->m_pArgs;

        flSpread = static_cast<float>(pArgs[kArgSpread].n);
        flSpeed  = static_cast<float>(pArgs[kArgSpeed].n);
    }

    auto* const pObject = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

    pEntity->GetPhysicsBody()->SetFlatOnTerrain(&pObject->m_pBody->prevPos);

    pEntity->SpawnParticles(flSpread, flSpeed);

    auto* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

    auto* const pOwner = pEntity->GetPlanState();

    pAgendaItem->Bind(pEntity, pOwner->m_pCurrent);
    pAgendaItem->Activate();
}

// The three aimed bursts differ in emitter direction: against, along, and world up (+Z); the up one spawns uniformly.

// FUNCTION: HYPERX 0x00402300
void PlanAction::ExplodeAgainstVelocity(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pTarget;

    float flSpread;
    float flSpeed;
    Vec3  vAgainst;

    auto* const pEntity = g_pGameEngine->GetParticleSystem(kSlotExplosion);

    const auto nArgCount = pNode->m_nArgCount;

    if (nArgCount < 2)
    {
        flSpread = DEFAULT_SPREAD;
        flSpeed  = DEFAULT_SPEED;
    }
    else if (nArgCount < 3)
    {
        flSpread = static_cast<float>(pNode->m_pArgs[kArgSpread].n);
        flSpeed  = DEFAULT_SPEED;
    }
    else
    {
        auto* const pArgs = pNode->m_pArgs;

        flSpread = static_cast<float>(pArgs[kArgSpread].n);
        flSpeed  = static_cast<float>(pArgs[kArgSpeed].n);
    }

    pTarget = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

    const Vec3* const pBody = &pTarget->m_pBody->vel;

    auto vDirection = *pBody;

    vAgainst.x = -vDirection.x;
    vAgainst.y = -vDirection.y;
    vAgainst.z = -vDirection.z;

    vDirection = vAgainst;

    pEntity->SetOrientation(pTarget->GetPositionPtr(), &vDirection);
    pEntity->SpawnParticles(flSpread, flSpeed);

    auto* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

    auto* const pOwner = pEntity->GetPlanState();

    pAgendaItem->Bind(pEntity, pOwner->m_pCurrent);
    pAgendaItem->Activate();
}

// FUNCTION: HYPERX 0x00402420
void PlanAction::ExplodeWithVelocity(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pTarget;

    float flSpread;
    float flSpeed;

    auto* const pEntity = g_pGameEngine->GetParticleSystem(kSlotExplosion);

    const auto nArgCount = pNode->m_nArgCount;

    if (nArgCount < 2)
    {
        flSpread = DEFAULT_SPREAD;
        flSpeed  = DEFAULT_SPEED;
    }
    else if (nArgCount < 3)
    {
        flSpread = static_cast<float>(pNode->m_pArgs[kArgSpread].n);
        flSpeed  = DEFAULT_SPEED;
    }
    else
    {
        auto* const pArgs = pNode->m_pArgs;

        flSpread = static_cast<float>(pArgs[kArgSpread].n);
        flSpeed  = static_cast<float>(pArgs[kArgSpeed].n);
    }

    pTarget = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

    const Vec3* const pBody = &pTarget->m_pBody->vel;

    const auto vDirection = *pBody;

    pEntity->SetOrientation(pTarget->GetPositionPtr(), &vDirection);
    pEntity->SpawnParticles(flSpread, flSpeed);

    auto* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

    auto* const pOwner = pEntity->GetPlanState();

    pAgendaItem->Bind(pEntity, pOwner->m_pCurrent);
    pAgendaItem->Activate();
}

// FUNCTION: HYPERX 0x00402510
void PlanAction::ExplodeUniform(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pTarget;

    float flSpread;
    float flSpeed;
    Vec3  vDirection;

    auto* const pEntity = g_pGameEngine->GetParticleSystem(kSlotExplosion);

    const auto nArgCount = pNode->m_nArgCount;

    if (nArgCount < 2)
    {
        flSpread = DEFAULT_SPREAD;
        flSpeed  = DEFAULT_SPEED;
    }
    else if (nArgCount < 3)
    {
        flSpread = static_cast<float>(pNode->m_pArgs[kArgSpread].n);
        flSpeed  = DEFAULT_SPEED;
    }
    else
    {
        auto* const pArgs = pNode->m_pArgs;

        flSpread = static_cast<float>(pArgs[kArgSpread].n);
        flSpeed  = static_cast<float>(pArgs[kArgSpeed].n);
    }

    pTarget = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

    vDirection.y = 0.0f;
    vDirection.x = 0.0f;
    vDirection.z = 1.0f;

    pEntity->SetOrientation(pTarget->GetPositionPtr(), &vDirection);
    pEntity->SpawnParticlesUniform(flSpread, flSpeed);

    auto* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

    auto* const pOwner = pEntity->GetPlanState();

    pAgendaItem->Bind(pEntity, pOwner->m_pCurrent);
    pAgendaItem->Activate();
}

// The two blood spurts fire only with brutality on and a free particle system; the small one lifts only a player's.

// FUNCTION: HYPERX 0x004025f0
void PlanAction::SmallBloodSpurt(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3 vStart;

    auto* const pActor = pContext->GetActor();

    if (g_pGameEngine->m_nBrutality != 0)
    {
        auto* const pSystem = g_pGameEngine->FindFreeParticleSystem();

        if (pSystem != nullptr)
        {
            vStart = *pActor->GetPositionPtr();

            const auto vDirection = pActor->m_pBody->vel;

            if (pActor->GetSlot() <= kGoalieSlotB)
            {
                vStart.z = vStart.GetZ() + 3.0f;
            }

            pSystem->SetOrientation(&vStart, &vDirection);
            pSystem->SpawnParticles(5.0f, 10.0f);

            auto* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

            auto* const pOwner = pSystem->GetPlanState();

            pAgendaItem->Bind(pSystem, pOwner->m_pCurrent);
            pAgendaItem->Activate();
        }
    }
}

// FUNCTION: HYPERX 0x004026d0
void PlanAction::LargeBloodSpurt(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3 vStart;

    auto* const pActor = pContext->GetActor();

    if (g_pGameEngine->m_nBrutality != 0)
    {
        auto* const pSystem = g_pGameEngine->FindFreeParticleSystem();

        if (pSystem != nullptr)
        {
            vStart = *pActor->GetPositionPtr();

            const auto vDirection = pActor->m_pBody->vel;

            vStart.z = vStart.GetZ() + 3.0f;

            pSystem->SetOrientation(&vStart, &vDirection);
            pSystem->SpawnParticles(10.0f, 25.0f);

            auto* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

            auto* const pOwner = pSystem->GetPlanState();

            pAgendaItem->Bind(pSystem, pOwner->m_pCurrent);
            pAgendaItem->Activate();
        }
    }
}

// The head bone, where the detached head starts.
#define HEAD_BONE 9

// Nought, copied into all three components of the point it transforms.
// GLOBAL: HYPERX 0x004b1458
extern const float g_flOriginComponent;

// Only the Rok does anything, and only onto a player's slot.
// FUNCTION: HYPERX 0x004027a0
void PlanAction::LoseHead(Interactor* pContext, PlanNode*, short*, int nArg)
{
    const auto nSlot = static_cast<unsigned int>(nArg);

    EngineObject* pActor;
    EngineObject* pSpawn;
    MeshBone*     pBone;

    PhysicsBody*     pSource;
    TransformObject* pActorMotion;
    TransformObject* pSpawnMotion;
    ParticleSystem*  pSystem;
    AgendaItem*      pAgendaItem;
    PlanState*       pOwner;
    Vec3             vVelocity;
    Vec3             vOrigin;
    Vec3             vScratch;

    pActor = pContext->GetActor();

    if (pActor->GetSlot() == kSlotRok)
    {
        pSpawn = g_pGameEngine->m_ppSlots[static_cast<short>(nSlot)];

        if (pSpawn != nullptr)
        {
            if (pSpawn->GetSlot() < kPlayerObjectCount)
            {
                ((RokObject*)pActor)->ActivateAtSpawnSlot(nSlot);

                pActor->GetPhysicsBody()->pAttach = nullptr;

                vOrigin.x = g_flOriginComponent;
                vOrigin.y = g_flOriginComponent;
                vOrigin.z = g_flOriginComponent;

                pBone = &pSpawn->m_pBones[HEAD_BONE];

                pBone->m_world.TransformPoint(vOrigin, &vScratch);

                const auto vHead = vScratch;

                vOrigin = vHead;

                pActor->m_pBody->pos = vHead;

                pBone->SetFlags(kMeshboneHidden);

                pSource   = pSpawn->GetPhysicsBody();
                vVelocity = pSource->velDir;
                vScratch  = pSpawn->m_pBody->vel;

                vVelocity.x = vVelocity.x + vScratch.x;
                vVelocity.y = vScratch.y + vVelocity.y;
                vVelocity.z = vVelocity.z + vScratch.z;

                Vec3::ScaleTo(&vVelocity, 2.0f, &vVelocity);

                vVelocity.z += 2.0f;

                pActor->SetVelocity(vVelocity);

                pActor->GetLocalTransformPtr()->ResetToIdentity();
                pActor->GetLocalTransformPtr()->SetTranslation(0, 0, 0);
                pActor->ApplyRenderSlotPose();

                pActorMotion = pActor->GetLocalTransformPtr();
                pSpawnMotion = pSpawn->GetLocalTransformPtr();

                pActorMotion->CopyRotation(pSpawnMotion);

                if (g_pGameEngine->m_nBrutality != 0)
                {
                    pSystem = g_pGameEngine->FindFreeParticleSystem();

                    if (pSystem != nullptr)
                    {
                        pSystem->SetOrientation(&vOrigin, &vVelocity);
                        pSystem->SpawnParticles(1.0f, 50.0f);

                        pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

                        pOwner = pSystem->GetPlanState();
                        pAgendaItem->Bind(pSystem, pOwner->m_pCurrent);
                        pAgendaItem->Activate();
                    }
                }
            }
        }
    }
}

// A player clears its own and its head bone's hidden flag; the Rok is put back on a spawn slot.
// FUNCTION: HYPERX 0x004029f0
void PlanAction::RegainHead(Interactor* pContext, PlanNode*, short*, int)
{
    auto* const pActor = pContext->GetActor();
    const int   nSlot  = pActor->GetSlot();

    auto* const pRok = g_pGameEngine->GetRok();

    if (nSlot < kSlotRok)
    {
        pActor->m_nFlags &= ~kEngineObjectHidden;

        pActor->m_pBones[HEAD_BONE].ClearFlags(kMeshboneHidden);

        return;
    }

    if (nSlot == kSlotRok)
    {
        pRok->ActivateAtSpawnSlot(kSpawnSlotAny);
    }
}

// The guards read the node's own arity.
#define ARG_OBJECT 1

constexpr auto kBoneSplit = 7;
constexpr auto kBoneCount = 15;
constexpr auto kHideLast  = 15;

short g_nSecondaryCamCursorA = -1;
short g_nSecondaryCamCursorB = -1;

// Slots four and five, the goalies, pass the guard and read bone indices as entity slots; the overrun is shipped.
// GLOBAL: HYPERX 0x004bb2e4
int g_anSubstitutePairs[8] = { 94, 95, 98, 99, 96, 97, 100, 101 };

// GLOBAL: HYPERX 0x004bb304
int g_anSubstituteBones[15] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

// The up axis the burst directions are crossed with; marked in src/globals.cpp.
extern const Vec3 g_vRokAxis;

// FUNCTION: HYPERX 0x00402a50
void PlanAction::BodySplit(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pSubject;
    EngineObject* pFirst;
    EngineObject* pSecond;
    const int*    pBone;
    int           nIndex;
    Vec3          vDirection;
    Vec3          vBack;

    if (pNode->m_nArgCount <= 2)
    {
        pSubject = pContext->GetActor();
    }
    else
    {
        const auto nId = pNode->m_pArgs[ARG_OBJECT].n;

        if (nId < 0)
        {
            return;
        }

        if (nId >= g_pGameEngine->m_nLoadedCount)
        {
            return;
        }

        pSubject = g_pGameEngine->m_ppSlots[static_cast<short>(nId)];
    }

    if (pSubject == nullptr)
    {
        return;
    }

    const int nSlot = pSubject->GetSlot();

    if (nSlot >= kPlayerObjectCount)
    {
        return;
    }

    const auto nPair = nSlot * 2;

    pFirst  = g_pGameEngine->m_ppSlots[static_cast<short>(g_anSubstitutePairs[nPair])];
    pSecond = g_pGameEngine->m_ppSlots[static_cast<short>(g_anSubstitutePairs[nPair + 1])];

    for (nIndex = 0; nIndex <= kHideLast; nIndex++)
    {
        if (pFirst != nullptr)
        {
            pFirst->m_pBones[static_cast<short>(nIndex)].ClearFlags(kMeshboneHidden);
        }

        if (pSecond != nullptr)
        {
            pSecond->m_pBones[static_cast<short>(nIndex)].ClearFlags(kMeshboneHidden);
        }
    }

    if (pNode->m_nArgCount >= 2)
    {
        if (pFirst != nullptr)
        {
            Object::SpawnAtObjectRotated(pSubject, pFirst, 1);
            pFirst->PostSceneEvent(kMsgDeke, 0);
        }

        if (pSecond != nullptr)
        {
            Object::SpawnAtObjectRotated(pSubject, pSecond, 0);
            pSecond->PostSceneEvent(kMsgDeke, 0);
        }

        return;
    }

    if (pFirst != nullptr)
    {
        for (pBone = g_anSubstituteBones; pBone < g_anSubstituteBones + kBoneSplit; pBone++)
        {
            pFirst->m_pBones[static_cast<short>(*pBone)].SetFlags(kMeshboneHidden);
        }

        Object::CopyPhysicsStateTo(pSubject, pFirst, 1);
    }

    if (pSecond != nullptr)
    {
        for (pBone = g_anSubstituteBones + kBoneSplit;
             pBone < g_anSubstituteBones + kBoneCount;
             pBone++)
        {
            pSecond->m_pBones[static_cast<short>(*pBone)].SetFlags(kMeshboneHidden);
        }

        Object::CopyPhysicsStateTo(pSubject, pSecond, 0);
    }

    if (g_pGameEngine->m_nBrutality != 0)
    {
        auto vPosition = *pSubject->GetPositionPtr();

        const auto vDir = pSubject->GetPhysicsBody()->velDir;

        const Vec3* const pDir = &vDir;

        for (nIndex = 0; nIndex < 3; nIndex++)
        {
            auto* const pSystem = g_pGameEngine->FindFreeParticleSystem();

            if (pSystem != nullptr)
            {
                switch (nIndex)
                {
                    case 0:
                        vDirection.x = pDir->z * g_vRokAxis.y - pDir->y * g_vRokAxis.z;
                        vDirection.y = pDir->x * g_vRokAxis.z - pDir->z * g_vRokAxis.x;
                        vDirection.z = pDir->y * g_vRokAxis.x - pDir->x * g_vRokAxis.y;
                        break;

                    case 1:
                        vDirection.x = pDir->y * g_vRokAxis.z - pDir->z * g_vRokAxis.y;
                        vDirection.y = pDir->z * g_vRokAxis.x - pDir->x * g_vRokAxis.z;
                        vDirection.z = pDir->x * g_vRokAxis.y - pDir->y * g_vRokAxis.x;
                        break;

                    default:
                        vBack.x = -pDir->x;
                        vBack.y = -pDir->y;
                        vBack.z = -pDir->z;

                        vDirection = vBack;
                        break;
                }

                vPosition.z += 2.0f;

                pSystem->SetOrientation(&vPosition, &vDirection);
                pSystem->SpawnParticles(1.0f, 50.0f);

                auto* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

                auto* const pPlan = pSystem->GetPlanState();

                pAgendaItem->Bind(pSystem, pPlan->m_pCurrent);
                pAgendaItem->Activate();
            }
        }
    }

    pSubject->m_nFlags |= kEngineObjectHidden;
}
