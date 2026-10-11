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
#include <math.h>
#include <stdlib.h>
#include <mbstring.h>
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
void __cdecl PlanAction::SetObjectToTrack(Interactor* pContext, PlanNode* pArgs, short*, int nSlot)
{
    short nValue;
    int   nJoystick;

    CameraObject* const pCamera = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);

    if (pCamera != 0)
    {
        const PlanWord* pBlock = pArgs->m_pArgs;
        if (pBlock[1].IsSelector())
        {
            PlanStatFn pfnSelector = g_apPlanStatFns[pBlock[1].wSelector];

            nValue = pfnSelector(pContext->GetActor(), pArgs);
        }
        else if (pBlock[1].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nValue = (short)nSlot;
        }
        else
        {
            nValue = (short)pBlock[1].n;
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

        if (nValue < PLAYER_OBJECT_COUNT)
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

    if (pContext != 0)
    {
        pObject = pContext->GetCameraActor();
    }
    else
    {
        pObject = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);
    }

    if (pObject->m_nPrimaryTracked == SLOT_ROK)
    {
        pTracked = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        if (pTracked == 0)
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

    if (pContext != 0)
    {
        pObject = pContext->GetCameraActor();
    }
    else
    {
        pObject = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);
    }

    // The actor's class here is not established; the other arm takes slot nine, a camera.
    pObject->m_nPrimaryTracked = g_pGameEngine->m_nViewedSlot;
}

// A count of two copies a marker's floats; otherwise the arguments are whole units.

// FUNCTION: HYPERX 0x004019a0
void __cdecl PlanAction::SetTrackingOffset(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    CameraObject* pActor;

    if (pArgs->m_nArgCount == 2)
    {
        Vec3* const pMarker = g_apTrackOffsets[pArgs->m_pArgs[1].n];

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->x = pMarker->x;

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->y = pMarker->y;

        pActor                 = pContext->GetCameraActor();
        pActor->m_pSettings->z = pMarker->z;

        return;
    }

    pActor                 = pContext->GetCameraActor();
    pActor->m_pSettings->x = (float)pArgs->m_pArgs[1].n;

    pActor                 = pContext->GetCameraActor();
    pActor->m_pSettings->y = (float)pArgs->m_pArgs[2].n;

    pActor                 = pContext->GetCameraActor();
    pActor->m_pSettings->z = (float)pArgs->m_pArgs[3].n;
}

// FUNCTION: HYPERX 0x00401a40
void __cdecl PlanAction::IncrementTrackingOffset(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    CameraObject* pActor;

    if (pArgs->m_nArgCount == 2)
    {
        Vec3* const pMarker = g_apTrackOffsets[pArgs->m_pArgs[1].n];

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
        (float)(pArgs->m_pArgs[1].n * 0.1 + pActor->m_pSettings->x);

    pActor = pContext->GetCameraActor();
    pActor->m_pSettings->y =
        (float)(pArgs->m_pArgs[2].n * 0.1 + pActor->m_pSettings->y);

    pActor = pContext->GetCameraActor();
    pActor->m_pSettings->z =
        (float)(pArgs->m_pArgs[3].n * 0.1 + pActor->m_pSettings->z);
}

// FUNCTION: HYPERX 0x00401b10
void __cdecl PlanAction::IncrementCameraDir(Interactor* pContext, PlanNode* pArgs, short*, int)
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
        pContext->GetActor()->GetPhysicsBody()->velDir.x += (float)(pArgs->m_pArgs[1].n * 0.01);
        pContext->GetActor()->GetPhysicsBody()->velDir.y += (float)(pArgs->m_pArgs[2].n * 0.01);

        pContext->GetActor()->GetPhysicsBody()->velDir.z += (float)(pArgs->m_pArgs[3].n * 0.01);
    }

    PhysicsBody* pBody = pContext->GetActor()->GetPhysicsBody();

    Math::Vec3Normalize(&pBody->velDir);
}

// FUNCTION: HYPERX 0x00401c00
void PlanAction::IncrementBungeeLength(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    float flValue;

    CameraObject* pActor = pContext->GetCameraActor();

    flValue = (float)(pArgs->m_pArgs[1].n * 0.1) + pActor->GetBungeeLength();

    pActor = pContext->GetCameraActor();

    pActor->m_flBungeeLength = flValue;
}

// FUNCTION: HYPERX 0x00401c40
void PlanAction::SetBungeeLength(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    const float flValue = (float)(pArgs->m_pArgs[1].n * 0.1);

    CameraObject* const pActor = pContext->GetCameraActor();

    pActor->m_flBungeeLength = flValue;
}

// Plan operations that write one setting each; the three camera heights and the buffer factor take tenths.
// FUNCTION: HYPERX 0x00401c70
void PlanAction::SetTrackHeight(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flCameraTrackingHeight = (float)(pArgs->m_pArgs[1].n * 0.1);
}

// FUNCTION: HYPERX 0x00401c90
void PlanAction::SetMinHeight(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flCameraMinHeight = (float)(pArgs->m_pArgs[1].n * 0.1);
}

// FUNCTION: HYPERX 0x00401cb0
void PlanAction::SetLookHeight(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flCameraLookHeight = (float)(pArgs->m_pArgs[1].n * 0.1);
}

// FUNCTION: HYPERX 0x00401cd0
void PlanAction::SetCamBufferFactor(Interactor*, PlanNode* pArgs, short*, int)
{
    g_flSteadyCamBuffer = (float)(pArgs->m_pArgs[1].n * 0.1);
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
    Vec3* const pPos = g_apTrackOffsets[pArgs->m_pArgs[1].n];

    pPos->x = (float)(pArgs->m_pArgs[2].n * 0.1);
    pPos->y = (float)(pArgs->m_pArgs[3].n * 0.1);
    pPos->z = (float)(pArgs->m_pArgs[4].n * 0.1);
}

#define TRACKED_SLOT_LIMIT 8

// Every rejection writes -1, so an invalid request clears the tracked object.
// FUNCTION: HYPERX 0x00401d70
void __cdecl PlanAction::SetObjectToWatch(Interactor* pContext, PlanNode* pArgs, short*, int nSlot)
{
    CameraObject* pTarget;
    int           nValue;

    if (pContext != 0)
    {
        pTarget = pContext->GetCameraActor();
    }
    else
    {
        pTarget = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);
    }

    const PlanWord* pBlock = pArgs->m_pArgs;
    if (pBlock[1].IsSelector())
    {
        PlanStatFn pfnSelector = g_apPlanStatFns[pBlock[1].wSelector];

        nValue = pfnSelector(pContext->GetActor(), pArgs);
    }
    else if (pBlock[1].n == PLAN_TOKEN_STRAIGHTAHEAD)
    {
        nValue = -1;
    }
    else if (pBlock[1].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nValue = (short)pBlock[1].n;
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

    if ((unsigned int)nValue > TRACKED_SLOT_LIMIT)
    {
        pTarget->m_nSecondaryTracked = -1;

        return;
    }

    if ((short)nValue > g_pGameEngine->m_nLoadedCount)
    {
        pTarget->m_nSecondaryTracked = -1;

        return;
    }

    if (g_pGameEngine->m_ppSlots[(short)nValue] == 0)
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

    g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->SetSecondaryTrackedObject(g_pGameEngine->m_ppObjects[g_nSecondaryCamCursorA]);
}

// FUNCTION: HYPERX 0x00401e90
void PlanAction::IncrementObjectToTrack(Interactor*, PlanNode*, short*, int)
{
    g_nSecondaryCamCursorB++;

    if (g_nSecondaryCamCursorB >= g_pGameEngine->m_nObjectCount - 1)
    {
        g_nSecondaryCamCursorB = 0;
    }

    g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->SetSecondaryTrackedObject(g_pGameEngine->m_ppObjects[g_nSecondaryCamCursorB]);
}

#define POS_WINDOW_SECONDS 0.5
#define DIR_WINDOW_SECONDS 0.4

// Sizes the smoothing windows from the frame rate; at the default factor the position one saturates above about 55 fps.
// FUNCTION: HYPERX 0x00401ee0
void PlanAction::ResetCameraBufferSize(Interactor*, PlanNode*, short*, int)
{
    GameEngine* pEngine = g_pGameEngine;

    float flRate   = pEngine->m_clock.flInvDelta;
    float flFactor = g_flSteadyCamBuffer;

    short nPos = (short)(flRate * flFactor * POS_WINDOW_SECONDS);

    if (nPos > CAMERA_HISTORY_CAPACITY)
    {
        nPos = CAMERA_HISTORY_CAPACITY;
    }
    else if (nPos < 1)
    {
        nPos = 1;
    }

    pEngine->GetCamera(CAMERA_MAIN_SLOT)->SetPosHistorySize(nPos);

    pEngine = g_pGameEngine;

    flRate   = pEngine->m_clock.flInvDelta;
    flFactor = g_flSteadyCamBuffer;

    short nDir = (short)(flRate * flFactor * DIR_WINDOW_SECONDS);

    if (nDir > CAMERA_HISTORY_CAPACITY)
    {
        nDir = CAMERA_HISTORY_CAPACITY;
    }
    else if (nDir < 1)
    {
        nDir = 1;
    }

    pEngine->GetCamera(CAMERA_MAIN_SLOT)->SetDirHistorySize(nDir);
}

#define ARG_COUNT_OWN_CAMERA    1
#define ARG_COUNT_SLOT_AND_MODE 3

#define ARG_SLOT 1
#define ARG_MODE 2

// A half; its marker is in src/globals.cpp.
extern const float g_flViewFracHalf;

// FUNCTION: HYPERX 0x00401f80
void __cdecl PlanAction::ResetReticle(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    CameraObject* pCamera;
    float         flFracX;
    float         flFracY;

    const short nArgCount = pArgs->m_nArgCount;

    if (nArgCount == ARG_COUNT_OWN_CAMERA)
    {
        flFracX = g_flViewFracHalf;
        pCamera = (CameraObject*)pContext->GetActor();
        pCamera->SetViewFracX(flFracX);

        flFracY = g_flViewFracHalf;
        pCamera = (CameraObject*)pContext->GetActor();
        pCamera->SetViewFracY(flFracY);

        ((CameraObject*)pContext->GetActor())->SetReticleState(RETICLE_AIM);
        return;
    }

    if (nArgCount == ARG_COUNT_SLOT_AND_MODE)
    {
        PlanWord* const pBlock = pArgs->m_pArgs;

        g_pGameEngine->GetCamera((short)pBlock[ARG_SLOT].n)->SetReticleState(pBlock[ARG_MODE].n);

        if (pArgs->m_pArgs[ARG_MODE].n == RETICLE_OFF)
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
    if (g_pRenderer != 0)
    {
        g_pRenderer->SetHeadCamText(pArgs->m_pArgs[1].psz);
    }

    ((CameraObject*)pContext->GetActor())->SetReticleState(RETICLE_OFF);
}

// Leads the tracked object by three frames of its velocity.

// FUNCTION: HYPERX 0x00402070
void __cdecl PlanAction::InitRearViewPos(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3* const pTarget = &pContext->GetActor()->GetPhysicsBody()->pos;

    EngineObject* const pTracked = pContext->GetCameraActor()->GetTrackedObject();

    if (pTracked != 0)
    {
        PhysicsBody* pState = pTracked->GetPhysicsBody();
        Vec3*        pPos   = pTracked->GetPositionPtr();
        pTarget->x          = g_flRearViewLeadFrames * pState->vel.x + pPos->x;

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
    CameraObject* const pCamera = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);

    const Vec3* const pPos      = &pCamera->GetPhysicsBody()->pos;
    Vec3* const       pSettings = pCamera->m_pSettings;

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

    pTarget                = g_pGameEngine->m_ppSlots[(short)pArgs->m_pArgs[1].n];
    Vec3* const pTargetPos = pTarget->GetPositionPtr();

    EngineObject* pActor   = pContext->GetActor();
    Vec3* const   pSelfPos = pActor->GetPositionPtr();

    pActor                     = pContext->GetActor();
    PhysicsBody* const pFacing = pActor->GetPhysicsBody();

    pActor              = pContext->GetActor();
    PhysicsBody* pBlock = pActor->GetPhysicsBody();

    Vec3* pVelocity = &pBlock->vel;

    vAim.x = pTargetPos->x - pSelfPos->x;
    vAim.y = pTargetPos->y - pSelfPos->y;
    vAim.z = pTargetPos->z - pSelfPos->z;

    Math::Vec3Normalize(&vAim);

    *pVelocity      = vAim;
    pFacing->velDir = pBlock->GetVel();

    flDelta = g_pGameEngine->m_clock.flDelta;

    pVelocity->ScaleInPlace(flDelta);
}

#define ARG_SPREAD 1
#define ARG_SPEED  2

// The defaults, taken whenever the block does not carry the number.
#define DEFAULT_SPREAD 20.0f
#define DEFAULT_SPEED  50.0f

// FUNCTION: HYPERX 0x00402230
void __cdecl PlanAction::ExplodeFlat(Interactor* pContext, PlanNode* pNode, short*, int)
{
    float flSpread;
    float flSpeed;

    ParticleSystem* const pEntity = g_pGameEngine->GetParticleSystem(SLOT_EXPLOSION);

    const short nArgCount = pNode->m_nArgCount;

    if (nArgCount < 2)
    {
        flSpread = DEFAULT_SPREAD;
        flSpeed  = DEFAULT_SPEED;
    }
    else if (nArgCount < 3)
    {
        flSpread = (float)pNode->m_pArgs[ARG_SPREAD].n;
        flSpeed  = DEFAULT_SPEED;
    }
    else
    {
        PlanWord* const pArgs = pNode->m_pArgs;

        flSpread = (float)pArgs[ARG_SPREAD].n;
        flSpeed  = (float)pArgs[ARG_SPEED].n;
    }

    EngineObject* pObject = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

    pEntity->GetPhysicsBody()->SetFlatOnTerrain(&pObject->m_pBody->prevPos);

    pEntity->SpawnParticles(flSpread, flSpeed);

    AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

    PlanState* const pOwner = pEntity->GetPlanState();

    pAgendaItem->Bind(pEntity, pOwner->m_pCurrent);
    pAgendaItem->Activate();
}

// The three aimed bursts differ in emitter direction: against, along, and world up (+Z); the up one spawns uniformly.

// FUNCTION: HYPERX 0x00402300
void __cdecl PlanAction::ExplodeAgainstVelocity(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pTarget;

    float flSpread;
    float flSpeed;
    Vec3  vAgainst;

    ParticleSystem* const pEntity = g_pGameEngine->GetParticleSystem(SLOT_EXPLOSION);

    const short nArgCount = pNode->m_nArgCount;

    if (nArgCount < 2)
    {
        flSpread = DEFAULT_SPREAD;
        flSpeed  = DEFAULT_SPEED;
    }
    else if (nArgCount < 3)
    {
        flSpread = (float)pNode->m_pArgs[ARG_SPREAD].n;
        flSpeed  = DEFAULT_SPEED;
    }
    else
    {
        PlanWord* const pArgs = pNode->m_pArgs;

        flSpread = (float)pArgs[ARG_SPREAD].n;
        flSpeed  = (float)pArgs[ARG_SPEED].n;
    }

    pTarget = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

    const Vec3* pBody = &pTarget->m_pBody->vel;

    Vec3 vDirection = *pBody;

    vAgainst.x = -vDirection.x;
    vAgainst.y = -vDirection.y;
    vAgainst.z = -vDirection.z;

    vDirection = vAgainst;

    pEntity->SetOrientation(pTarget->GetPositionPtr(), &vDirection);
    pEntity->SpawnParticles(flSpread, flSpeed);

    AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

    PlanState* const pOwner = pEntity->GetPlanState();

    pAgendaItem->Bind(pEntity, pOwner->m_pCurrent);
    pAgendaItem->Activate();
}

// FUNCTION: HYPERX 0x00402420
void __cdecl PlanAction::ExplodeWithVelocity(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pTarget;

    float flSpread;
    float flSpeed;

    ParticleSystem* const pEntity = g_pGameEngine->GetParticleSystem(SLOT_EXPLOSION);

    const short nArgCount = pNode->m_nArgCount;

    if (nArgCount < 2)
    {
        flSpread = DEFAULT_SPREAD;
        flSpeed  = DEFAULT_SPEED;
    }
    else if (nArgCount < 3)
    {
        flSpread = (float)pNode->m_pArgs[ARG_SPREAD].n;
        flSpeed  = DEFAULT_SPEED;
    }
    else
    {
        PlanWord* const pArgs = pNode->m_pArgs;

        flSpread = (float)pArgs[ARG_SPREAD].n;
        flSpeed  = (float)pArgs[ARG_SPEED].n;
    }

    pTarget = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

    const Vec3* pBody = &pTarget->m_pBody->vel;

    Vec3 vDirection = *pBody;

    pEntity->SetOrientation(pTarget->GetPositionPtr(), &vDirection);
    pEntity->SpawnParticles(flSpread, flSpeed);

    AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

    PlanState* const pOwner = pEntity->GetPlanState();

    pAgendaItem->Bind(pEntity, pOwner->m_pCurrent);
    pAgendaItem->Activate();
}

// FUNCTION: HYPERX 0x00402510
void __cdecl PlanAction::ExplodeUniform(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pTarget;

    float flSpread;
    float flSpeed;
    Vec3  vDirection;

    ParticleSystem* const pEntity = g_pGameEngine->GetParticleSystem(SLOT_EXPLOSION);

    const short nArgCount = pNode->m_nArgCount;

    if (nArgCount < 2)
    {
        flSpread = DEFAULT_SPREAD;
        flSpeed  = DEFAULT_SPEED;
    }
    else if (nArgCount < 3)
    {
        flSpread = (float)pNode->m_pArgs[ARG_SPREAD].n;
        flSpeed  = DEFAULT_SPEED;
    }
    else
    {
        PlanWord* const pArgs = pNode->m_pArgs;

        flSpread = (float)pArgs[ARG_SPREAD].n;
        flSpeed  = (float)pArgs[ARG_SPEED].n;
    }

    pTarget = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

    vDirection.y = 0.0f;
    vDirection.x = 0.0f;
    vDirection.z = 1.0f;

    pEntity->SetOrientation(pTarget->GetPositionPtr(), &vDirection);
    pEntity->SpawnParticlesUniform(flSpread, flSpeed);

    AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

    PlanState* const pOwner = pEntity->GetPlanState();

    pAgendaItem->Bind(pEntity, pOwner->m_pCurrent);
    pAgendaItem->Activate();
}

// The two blood spurts fire only with brutality on and a free particle system; the small one lifts only a player's.

// FUNCTION: HYPERX 0x004025f0
void __cdecl PlanAction::SmallBloodSpurt(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3 vStart;

    EngineObject* const pActor = pContext->GetActor();

    if (g_pGameEngine->m_nBrutality != 0)
    {
        ParticleSystem* const pSystem = g_pGameEngine->FindFreeParticleSystem();

        if (pSystem != 0)
        {
            vStart = *pActor->GetPositionPtr();

            Vec3 vDirection = pActor->m_pBody->vel;

            if (pActor->GetSlot() <= GOALIE_SLOT_B)
            {
                vStart.z = vStart.GetZ() + 3.0f;
            }

            pSystem->SetOrientation(&vStart, &vDirection);
            pSystem->SpawnParticles(5.0f, 10.0f);

            AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

            PlanState* const pOwner = pSystem->GetPlanState();

            pAgendaItem->Bind(pSystem, pOwner->m_pCurrent);
            pAgendaItem->Activate();
        }
    }
}

// FUNCTION: HYPERX 0x004026d0
void __cdecl PlanAction::LargeBloodSpurt(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3 vStart;

    EngineObject* const pActor = pContext->GetActor();

    if (g_pGameEngine->m_nBrutality != 0)
    {
        ParticleSystem* const pSystem = g_pGameEngine->FindFreeParticleSystem();

        if (pSystem != 0)
        {
            vStart = *pActor->GetPositionPtr();

            Vec3 vDirection = pActor->m_pBody->vel;

            vStart.z = vStart.GetZ() + 3.0f;

            pSystem->SetOrientation(&vStart, &vDirection);
            pSystem->SpawnParticles(10.0f, 25.0f);

            AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

            PlanState* const pOwner = pSystem->GetPlanState();

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
void __cdecl PlanAction::LoseHead(Interactor* pContext, PlanNode*, short*, int nArg)
{
    const unsigned int nSlot = (unsigned int)nArg;

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

    if (pActor->GetSlot() == SLOT_ROK)
    {
        pSpawn = g_pGameEngine->m_ppSlots[(short)nSlot];

        if (pSpawn != 0)
        {
            if (pSpawn->GetSlot() < PLAYER_OBJECT_COUNT)
            {
                ((RokObject*)pActor)->ActivateAtSpawnSlot(nSlot);

                pActor->GetPhysicsBody()->pAttach = 0;

                vOrigin.x = g_flOriginComponent;
                vOrigin.y = g_flOriginComponent;
                vOrigin.z = g_flOriginComponent;

                pBone = &pSpawn->m_pBones[HEAD_BONE];

                pBone->m_world.TransformPoint(vOrigin, &vScratch);

                const Vec3 vHead = vScratch;

                vOrigin = vHead;

                pActor->m_pBody->pos = vHead;

                pBone->SetFlags(MESHBONE_HIDDEN);

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

                    if (pSystem != 0)
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
void __cdecl PlanAction::RegainHead(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* const pActor = pContext->GetActor();
    const int           nSlot  = pActor->GetSlot();

    RokObject* const pRok = g_pGameEngine->GetRok();

    if (nSlot < SLOT_ROK)
    {
        pActor->m_nFlags &= ~ENGINEOBJECT_HIDDEN;

        pActor->m_pBones[HEAD_BONE].ClearFlags(MESHBONE_HIDDEN);

        return;
    }

    if (nSlot == SLOT_ROK)
    {
        pRok->ActivateAtSpawnSlot(SPAWN_SLOT_ANY);
    }
}

// The guards read the node's own arity.
#define ARG_OBJECT 1

#define BONE_SPLIT 7
#define BONE_COUNT 15
#define HIDE_LAST  15

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
void __cdecl PlanAction::BodySplit(Interactor* pContext, PlanNode* pNode, short*, int)
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
        const int nId = pNode->m_pArgs[ARG_OBJECT].n;

        if (nId < 0)
        {
            return;
        }

        if (nId >= g_pGameEngine->m_nLoadedCount)
        {
            return;
        }

        pSubject = g_pGameEngine->m_ppSlots[(short)nId];
    }

    if (pSubject == 0)
    {
        return;
    }

    const int nSlot = pSubject->GetSlot();

    if (nSlot >= PLAYER_OBJECT_COUNT)
    {
        return;
    }

    const int nPair = nSlot * 2;

    pFirst  = g_pGameEngine->m_ppSlots[(short)g_anSubstitutePairs[nPair]];
    pSecond = g_pGameEngine->m_ppSlots[(short)g_anSubstitutePairs[nPair + 1]];

    for (nIndex = 0; nIndex <= HIDE_LAST; nIndex++)
    {
        if (pFirst != 0)
        {
            pFirst->m_pBones[(short)nIndex].ClearFlags(MESHBONE_HIDDEN);
        }

        if (pSecond != 0)
        {
            pSecond->m_pBones[(short)nIndex].ClearFlags(MESHBONE_HIDDEN);
        }
    }

    if (pNode->m_nArgCount >= 2)
    {
        if (pFirst != 0)
        {
            Object::SpawnAtObjectRotated(pSubject, pFirst, 1);
            pFirst->PostSceneEvent(MSG_DEKE, 0);
        }

        if (pSecond != 0)
        {
            Object::SpawnAtObjectRotated(pSubject, pSecond, 0);
            pSecond->PostSceneEvent(MSG_DEKE, 0);
        }

        return;
    }

    if (pFirst != 0)
    {
        for (pBone = g_anSubstituteBones; pBone < g_anSubstituteBones + BONE_SPLIT; pBone++)
        {
            pFirst->m_pBones[(short)*pBone].SetFlags(MESHBONE_HIDDEN);
        }

        Object::CopyPhysicsStateTo(pSubject, pFirst, 1);
    }

    if (pSecond != 0)
    {
        for (pBone = g_anSubstituteBones + BONE_SPLIT;
             pBone < g_anSubstituteBones + BONE_COUNT;
             pBone++)
        {
            pSecond->m_pBones[(short)*pBone].SetFlags(MESHBONE_HIDDEN);
        }

        Object::CopyPhysicsStateTo(pSubject, pSecond, 0);
    }

    if (g_pGameEngine->m_nBrutality != 0)
    {
        Vec3 vPosition = *pSubject->GetPositionPtr();

        Vec3 vDir = pSubject->GetPhysicsBody()->velDir;

        const Vec3* pDir = &vDir;

        for (nIndex = 0; nIndex < 3; nIndex++)
        {
            ParticleSystem* const pSystem = g_pGameEngine->FindFreeParticleSystem();

            if (pSystem != 0)
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

                AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

                PlanState* const pPlan = pSystem->GetPlanState();

                pAgendaItem->Bind(pSystem, pPlan->m_pCurrent);
                pAgendaItem->Activate();
            }
        }
    }

    pSubject->m_nFlags |= ENGINEOBJECT_HIDDEN;
}
