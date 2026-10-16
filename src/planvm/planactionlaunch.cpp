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
#include "common/staticinit.h"
#include "engine/objectstats.h"
#include "damagegate.h"
#include "physics/boundbubble.h"
#include "graphics/meshboundingbox.h"
#include "engine/teamblock.h"
#include "common/stub.h"
#include "planarg.h"
#include "engine/statids.h"
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
#include "engine/objectphase.h"
#include "planvm/shotword.h"

extern const float g_flZeroPlanLaunch    = 0.0f;
float              g_flUnusedPlanLaunchA = g_flZeroPlanLaunch;
float              g_flUnusedPlanLaunchB = g_flZeroPlanLaunch;
float              g_flUnusedPlanLaunchC = g_flZeroPlanLaunch;

#define SHOT_POWER_RESET 0x32

// Range byte -1: Launch takes its unaimed branch.
#define SHOT_UNAIMED 0xffff0000

// The human packs the head camera's aim and shot power into the shot; any other plan sends it unaimed.
// FUNCTION: HYPERX 0x004031e0
void __cdecl PlanAction::SendLaunchMessage(Interactor* pContext, PlanNode*, short*, int)
{
    unsigned int nPacked;

    EngineObject* const pTarget = pContext->GetActor();

    if (pTarget->GetPhase() == OBJECT_PHASE_LOCAL)
    {
        const unsigned char* pszName = (const unsigned char*)pTarget->GetPlanState()->m_pszName;

        // STRING: HYPERX 0x004bb340
        if (_mbscmp((const unsigned char*)"Human", pszName) == 0)
        {
            CameraObject* const pHeadCam = g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT);

            nPacked = ShotWord::Pack((char)(int)(pHeadCam->GetViewFracX() * 100.0f),
                                     (char)pHeadCam->m_nShotPowerLevel,
                                     (char)(int)(pHeadCam->GetViewFracY() * 100.0f));

            pHeadCam->m_nShotPowerLevel = SHOT_POWER_RESET;
        }
        else
        {
            nPacked = SHOT_UNAIMED;
        }

        pTarget->PostSceneMessage(MSG_SHOOT, nPacked, pTarget->GetSlot(), 0);
    }
}

// Three, divided into the pi beside it.
// GLOBAL: HYPERX 0x004b148c
extern const float g_flThreeThrow;

// The packed parameter holds three signed bytes: elevation, spin and range gate.
// FUNCTION: HYPERX 0x00403290
void __cdecl PlanAction::Launch(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    const ShotWord shot = { (unsigned int)nArg };

    EngineObject* pRok;

    if (pContext->GetNamedSlot() >= 0)
    {
        pRok = pContext->GetActor();
    }
    else
    {
        pRok = g_pGameEngine->m_ppSlots[(short)CollisionObj::GetModelId(pContext->GetModelSlot())];
    }

    Vec3* const pVel          = &pRok->GetPhysicsBody()->velDir;
    float*      pflSpeedScale = &pRok->GetPhysicsBody()->flSpeed;
    const float flMaxSpeed    = pRok->GetPhysicsBody()->flForwardSpeedDt;

    if (pRok->GetSlot() >= SLOT_ROK)
    {
        PlayerObject* pCarrier;
        ObjectStats*  pStats;
        AttachPak*    pAttach;
        short         nWanted;
        Vec3*         pCarrierDir;
        Vec3*         pGoalPos;
        Vec3*         pCarrierPos;
        Vec3*         pDiff;
        Vec3          vDiff;
        Vec3          vGoal;
        float         flRange;
        float         flSpin;
        float         flDistance;
        float         flThird;
        float         flLimit;

        nWanted = pContext->GetTargetSlot();

        if (pRok->GetPhase() != OBJECT_PHASE_LOCAL)
        {
            return;
        }

        pAttach = pRok->GetPhysicsBody()->pAttach;

        if (pAttach == 0)
        {
            return;
        }

        if (nWanted != pAttach->nSlot && nWanted != pContext->GetActor()->GetSlot())
        {
            return;
        }

        pCarrier    = g_pGameEngine->GetPlayer(pAttach->nSlot);
        pCarrierDir = &pCarrier->GetPhysicsBody()->velDir;

        pGoalPos = g_pGameEngine->m_ppSlots[pCarrier->m_wTargetGoalSlot]->GetPositionPtr();

        vGoal  = *pGoalPos;
        pStats = &pCarrier->m_stats;

        pCarrierPos = pCarrier->GetPositionPtr();

        vDiff.x = vGoal.x - pCarrierPos->x;
        vDiff.y = vGoal.y - pCarrierPos->y;
        vDiff.z = vGoal.z - pCarrierPos->z;

        flRange = (float)shot.GetPower();
        flSpin  = (float)shot.GetAimX() * 0.01f;

        pDiff = &vDiff;

        flDistance = (float)sqrt(pDiff->y * pDiff->y + (pDiff->GetX() * pDiff->x + pDiff->z * pDiff->z));

        Math::Vec3Normalize(&vDiff);

        flThird = g_flPiThrow / g_flThreeThrow;
        flLimit = (250.0f - flDistance) * flThird * 0.004f;
        flLimit += flThird;

        if ((float)acos(vDiff.z * pCarrierDir->z + vDiff.y * pCarrierDir->y + vDiff.x * pCarrierDir->x) > flLimit)
        {
            Vec3* pCarrierVel;
            float flDelta;
            float flPower;
            float flGain;

            pCarrierVel = &pCarrier->GetPhysicsBody()->velDir;

            pCarrierVel->z = (float)fabs(pCarrierVel->z) * 0.5f + pCarrierVel->z;

            flDelta = g_pGameEngine->m_clock.flDelta;

            flPower = pStats->GetScalar(RATING_SHOT_POWER) * pStats->GetScalar(RATING_OFFENSIVE_ABILITY);

            flGain = ((float)rand() * flPower * (1.0f / RAND_MAX) + 10.0f) * flDelta;

            *pflSpeedScale = flGain * *pflSpeedScale;

            Vec3::ScaleTo(pCarrierVel, *pflSpeedScale, &pRok->GetPhysicsBody()->vel);
            return;
        }

        if (flRange >= 0.0f)
        {
            Vec3  vAxis;
            Vec3  vBase;
            Vec3  vSwing;
            Vec3* pRokVel;
            float flDrop;
            float flSide;
            float flLength;

            float flRoot;
            float flHalf;
            float flCap;
            float flReference;
            float flScale;
            float flInvScale;
            float flDelta;

            vAxis.x = g_flZeroPlanLaunch;
            vAxis.y = g_flZeroPlanLaunch;
            vAxis.z = g_flOneThrow;

            pVel->x = -pRok->GetPositionPtr()->x;

            flDrop = (0.5f - (float)shot.GetAimY() * 0.01f) * 16.0f;

            pVel->z = flDrop - pRok->GetPositionPtr()->z - 10.0f;

            if (pCarrier->m_wTargetGoalSlot == SLOT_SGOAL)
            {
                flSide  = -g_arenaEllipsoid.GetRadiusY();
                pVel->y = flSide - pRok->GetPositionPtr()->y;
            }
            else
            {
                flSide  = g_arenaEllipsoid.GetRadiusY();
                pVel->y = flSide - pRok->GetPositionPtr()->y;
            }

            vBase = *pVel;

            vSwing.x = vBase.y * vAxis.z - vBase.z * vAxis.y;
            vSwing.y = vBase.z * vAxis.x;
            vSwing.z = -(vBase.y * vAxis.x);

            Math::Vec3Normalize(&vSwing);
            vSwing.ScaleInPlace((flSpin - 0.5f) * 16.0f);

            pVel->x = vSwing.x + pVel->x;
            pVel->y = vSwing.y + pVel->y;
            pVel->z = vSwing.z + pVel->z;

            flHalf = (flRange + 175.0f) * 0.5f;

            flRoot = (float)sqrt(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z);

            flLength = flRoot;

            if (pStats->GetScalar(RATING_LONG_SHOT_ENABLE) != 0.0f)
            {
                flCap       = g_arenaEllipsoid.GetRadiusY() + g_arenaEllipsoid.GetRadiusY();
                flReference = flMaxSpeed * 5.0f;
            }
            else
            {
                flCap       = 150.0f;
                flReference = flMaxSpeed;
            }

            flDelta = g_pGameEngine->m_clock.flDelta;

            flScale    = (flHalf * 0.015f + 0.1f) * flReference / flLength;
            flInvScale = 1.0f / flScale;

            pVel->x = flScale * pVel->x;
            pVel->y = flScale * pVel->y;

            if (flCap - flLength < g_flZeroPlanLaunch)
            {
                pVel->z = flScale * pVel->z;
            }
            else
            {
                const float flRise = flInvScale * flDelta * g_pGameEngine->m_clock.flDelta * 32.0f;

                pVel->z = flRise * 0.5f + flScale * pVel->z;
            }

            pRokVel  = &pRok->GetPhysicsBody()->vel;
            *pRokVel = *pVel;

            if (pRok->GetSlot() != SLOT_ROK)
            {
                return;
            }

            g_pGameEngine->m_ppSlots[pCarrier->m_wTargetGoalSlot]
                ->PostSceneEvent(MSG_SHOT_ON_GOAL, 0);

            g_pGameEngine->m_ppSlots[(short)(pCarrier->m_wTargetGoalSlot - 3)]
                ->PostSceneEvent(MSG_SHOT_ON_GOAL, (int)flInvScale);
            return;
        }
        else
        {
            PhysicsBody* pRokBlock;
            float        flDelta;
            float        flJitterX;
            float        flJitterZ;
            float        flSide;
            float        flFlight;

            float flRoot;

            flDelta = g_pGameEngine->m_clock.flDelta;

            *pflSpeedScale = (((float)rand() * (0.5f / RAND_MAX) + 0.5f) * pStats->GetScalar(RATING_OFFENSIVE_ABILITY) * 10.0f + 75.0f) * flDelta;

            if (g_bSessionActive != 0)
            {
                *pflSpeedScale = *pflSpeedScale * 1.5f;
            }

            flJitterX = (float)rand() * (1.0f / RAND_MAX) - 0.5f;
            flJitterZ = (float)rand() * (1.0f / RAND_MAX) - 0.5f;

            pVel->x = flJitterX * 10.0f - pRok->GetPositionPtr()->x;

            if (pCarrier->m_wTargetGoalSlot == SLOT_SGOAL)
            {
                flSide  = -g_arenaEllipsoid.GetRadiusY();
                pVel->y = flSide - pRok->GetPositionPtr()->y;
            }
            else
            {
                flSide  = g_arenaEllipsoid.GetRadiusY();
                pVel->y = flSide - pRok->GetPositionPtr()->y;
            }

            pVel->z = flJitterZ * 10.0f - pRok->GetPositionPtr()->z - 20.0f;

            flRoot = (float)sqrt(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z);

            flFlight = flRoot / *pflSpeedScale;

            const float flGravity = g_pGameEngine->m_clock.flDeltaSquared * 32.0f;

            pVel->z = flGravity * flFlight * flFlight + pVel->z;

            pVel->NormalizeTo(pVel);

            pRokBlock = pRok->GetPhysicsBody();
            Vec3::ScaleTo(pVel, *pflSpeedScale, &pRokBlock->vel);

            if (pRok->GetSlot() != SLOT_ROK)
            {
                return;
            }

            g_pGameEngine->m_ppSlots[pCarrier->m_wTargetGoalSlot]
                ->PostSceneEvent(MSG_SHOT_ON_GOAL, 0);

            g_pGameEngine->m_ppSlots[(short)(pCarrier->m_wTargetGoalSlot - 3)]->PostSceneEvent(MSG_SHOT_ON_GOAL, 0);
            return;
        }
    }
    else
    {
        Vec3*        pRokVel;
        PhysicsBody* pRokBody;
        float        flSpeed;
        float        flBump;

        pRokVel = &pRok->GetPhysicsBody()->vel;

        flSpeed = (float)sqrt(pRokVel->x * pRokVel->x + pRokVel->y * pRokVel->y + pRokVel->z * pRokVel->z);

        flBump = g_pGameEngine->m_clock.flDelta * 10.0f;

        if (flBump <= flSpeed)
        {
            flBump = flSpeed * 0.5f;
        }

        pRokVel->z = flBump + pRokVel->z;

        pRok->PostSceneEvent(MSG_FLY, 0);

        pRokBody = pRok->GetPhysicsBody();
        Vec3::ScaleTo(pRokVel, pArgs->m_pArgs[1].fl, &pRokBody->vel);
    }
}

// Hands the rok the velocity of whatever the camera is watching, behind four gates.
// FUNCTION: HYPERX 0x00403a30
void __cdecl PlanAction::Throw(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3*  pVel   = &pContext->GetActor()->GetPhysicsBody()->velDir;
    float* pSpeed = &pContext->GetActor()->GetPhysicsBody()->flSpeed;

    if (pContext->GetActor()->GetSlot() == SLOT_ROK && pContext->GetActor()->GetPhase() == OBJECT_PHASE_LOCAL)
    {
        PhysicsBody*     pBlock   = pContext->GetActor()->GetPhysicsBody();
        AttachPak* const pTracked = pBlock->pAttach;

        if (pTracked != 0)
        {
            PlayerObject* const pTarget = g_pGameEngine->GetPlayer(pTracked->nSlot);
            const short         nViewed = g_pGameEngine->m_nViewedSlot;

            if (pTarget->GetSlot() == nViewed)
            {
                *pVel = pTarget->GetPhysicsBody()->velDir;

                pContext->GetActor()->GetPhysicsBody();

                const float flDt = g_pGameEngine->m_clock.flDelta;

                *pSpeed = ((pTarget->m_stats.GetWeightedStat(RATING_SHOT_POWER) * 0.04f + 0.8f) * 110.0f) * flDt;

                pVel->z += 0.5f;

                Math::Vec3Normalize(pVel);

                pBlock = pContext->GetActor()->GetPhysicsBody();

                Vec3::ScaleTo(pVel, *pSpeed, &pBlock->vel);
            }
        }
    }
}

// Pushes the vertical velocity down and never lets it rise, and reverses and quarters the horizontal pair.

// Ten units a second, and the two scale factors.
#define DAMP_RATE        10.0f
#define VERTICAL_HALVING 0.5f
#define HORIZONTAL_SCALE (-0.25f)

// FUNCTION: HYPERX 0x00403cf0
void __cdecl PlanAction::KickDownToSurface(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* const pActor = pContext->GetActor();
    PhysicsBody*        pBlock = pActor->GetPhysicsBody();

    Vec3* const pVelocity = &pBlock->vel;

    const float flStep = g_pGameEngine->m_clock.flDelta * DAMP_RATE;

    if (pVelocity->z > 0.0f)
    {
        pVelocity->z *= VERTICAL_HALVING;
    }

    pVelocity->z -= flStep;

    if (pVelocity->z > 0.0f)
    {
        pVelocity->z = 0.0f;
    }

    pVelocity->x *= HORIZONTAL_SCALE;
    pVelocity->y *= HORIZONTAL_SCALE;
}

// The goalies lead the target by the Rok's flight time to their goal, and add scatter.

#define DICE_WIDE   7
#define DICE_NARROW 6
// The span the bias below centres, twenty one wide.
#define JITTER_SPAN 21
#define JITTER_BIAS 10

// FUNCTION: HYPERX 0x00403d70
void __cdecl PlanAction::LaunchAt(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    Vec3* pGoal;
    Vec3  vDiff;
    Vec3  vJitter;
    float flSpeed;
    float flScratchA;
    float flFlight;
    float flScratchB;

    EngineObject* pActor = pContext->GetActor();

    const int nIndex = pArgs->m_pArgs[1].n;

    EngineObject* const pTarget = g_pGameEngine->m_ppSlots[(short)nIndex];

    Vec3* const pActorPos = pActor->GetPositionPtr();

    Vec3 vTargetPos = *pTarget->GetPositionPtr();

    Vec3* const pOut = &pActor->GetPhysicsBody()->vel;

    pActor->GetPhysicsBody();

    Vec3 vTargetVel = pTarget->GetPhysicsBody()->vel;

    const float flTargetScale = pTarget->GetPhysicsBody()->flSpeed;

    int nSlot = pActor->GetSlot();

    EngineObject* const pRok = g_pGameEngine->m_ppSlots[SLOT_ROK];

    if (pActor->GetSlot() % 2 == 0)
    {
        pGoal = g_apPositionMarkers[POS_N_GOAL_CENTER];
    }
    else
    {
        pGoal = g_apPositionMarkers[POS_S_GOAL_CENTER];
    }

    Vec3* const pRokPos = pRok->GetPositionPtr();

    vDiff.x = pGoal->x - pRokPos->x;
    vDiff.y = pGoal->y - pRokPos->y;
    vDiff.z = pGoal->z - pRokPos->z;

    Vec3 vRokVel = pRok->GetPhysicsBody()->vel;

    if (nSlot == GOALIE_SLOT_A || nSlot == GOALIE_SLOT_B)
    {
        flScratchA = (float)sqrt(Vec3::DotInline(&vDiff, &vDiff));

        flFlight = (flScratchA / (float)sqrt(Vec3::DotInline(&vRokVel, &vRokVel))) * g_pGameEngine->m_clock.flDelta;

        if (nIndex == SLOT_ROK)
        {
            float flSteps;

            flSteps = flScratchA * 0.016666667535901070f;

            if (g_flThrowStepsCap <= flSteps || 1.0f <= flSteps)
            {
                if (g_flThrowStepsCap < flSteps)
                {
                    flSteps = g_flThrowStepsCap;
                }
            }
            else
            {
                flSteps = 1.0f;
            }

            flSpeed = (flSteps * 0.07999999821186066f + 0.6499999761581421f) * flFlight;
        }
        else
        {
            flSpeed = flFlight * 0.25f;
        }
    }
    else
    {
        flSpeed = 0.5f;
    }

    flScratchA = flSpeed * g_pGameEngine->m_clock.GetInvDelta();

    if (flTargetScale * g_pGameEngine->m_clock.GetInvDelta() > 1.0f)
    {
        vTargetVel.ScaleInPlace(flScratchA);

        vTargetPos.x = vTargetPos.x + vTargetVel.x;
        vTargetPos.y = vTargetPos.y + vTargetVel.y;
        vTargetPos.z = vTargetPos.z + vTargetVel.z;
    }

    if (nSlot == GOALIE_SLOT_A || nSlot == GOALIE_SLOT_B)
    {
        PlayerObject* pOwner;
        float         flHandling;
        float         flReach;
        float         flDamageA;
        float         flDamageB;
        float         flGate;
        float         flDice;
        float         flScatter;

        pOwner     = pContext->GetPlayerActor();
        flHandling = pOwner->m_stats.GetScalar(RATING_ROK_HANDLING);

        flReach = (float)sqrt(Vec3::DotInline(&vDiff, &vDiff)) * 0.01f;

        if (2.0f <= flReach || 1.0f <= flReach)
        {
            if (2.0f < flReach)
            {
                flReach = 2.0f;
            }
        }
        else
        {
            flReach = 1.0f;
        }

        flScratchB = flReach * flHandling;

        nSlot = nSlot % 2;

        pOwner    = g_pGameEngine->GetPlayer((short)nSlot);
        flDamageA = pOwner->m_stats.GetScalar(RATING_DAMAGE);

        nSlot += 2;

        pOwner    = g_pGameEngine->GetPlayer((short)nSlot);
        flDamageB = pOwner->m_stats.GetScalar(RATING_DAMAGE);

        flGate = DamageGate();

        if (flDamageA <= flGate)
        {
            if (flDamageB <= DamageGate())
            {
                flScratchB = 0.0f;
            }
        }

        flDice = (float)(rand() % DICE_WIDE);
        flDice = (float)(rand() % DICE_WIDE) + flDice;
        flDice = (float)(rand() % DICE_NARROW) + flDice - flScratchB;

        if (1.0f > flDice)
        {
            flDice = 1.0f;
        }

        flScatter = flDice * 0.10000000149011612f;

        vJitter.x = (float)(rand() % JITTER_SPAN - JITTER_BIAS);
        vJitter.y = (float)(rand() % JITTER_SPAN - JITTER_BIAS);
        vJitter.z = (float)(rand() % JITTER_SPAN - JITTER_BIAS);

        Math::Vec3Normalize(&vJitter);

        if (vJitter.z > 0.0f)
        {
            vJitter.z = -vJitter.z;
        }

        vJitter.ScaleInPlace(flScatter);
    }

    pOut->x = vTargetPos.x - pActorPos->x;
    pOut->y = vTargetPos.y - pActorPos->y;
    pOut->z = vTargetPos.z - pActorPos->z;

    // Without the scatter the jitter is read unset, as the original does.
    vJitter.ScaleInPlace(
        (float)sqrt((pOut->x * pOut->x + pOut->y * pOut->y) + pOut->z * pOut->z) * 0.6000000238418579f);

    pOut->x = vJitter.x + pOut->x;
    pOut->y = vJitter.y + pOut->y;
    pOut->z = vJitter.z + pOut->z;

    pOut->ScaleInPlace(g_flOneThrow / flScratchA);

    flScratchB = pActor->GetPhysicsBody()->flForwardSpeedDt;

    if ((float)sqrt((pOut->x * pOut->x + pOut->y * pOut->y) + pOut->z * pOut->z) > flScratchB)
    {
        Math::Vec3Normalize(pOut);
        pOut->ScaleInPlace(flScratchB);
    }
}

// The sibling of Throw with different constants; a speed at or below nought is forced to ten.

#define ARG_COUNT_WITHOUT_OPERAND 1
#define ARG_OPERAND               1

#define SPEED_FLOOR 10.0f

// FUNCTION: HYPERX 0x00404320
void __cdecl PlanAction::LaunchToward(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    const PlanWord param = { nArg };

    EngineObject* pTarget;
    unsigned int  nOperand;

    Vec3* const pAxis  = &pContext->GetActor()->GetPhysicsBody()->velDir;
    float*      pSpeed = &pContext->GetActor()->GetPhysicsBody()->flSpeed;

    if (pContext->GetNamedSlot() >= 0)
    {
        pContext->GetActor();
    }
    else
    {
        CollisionObj::GetModelId(pContext->GetModelSlot());
    }

    if (param.IsPronoun())
    {
        nOperand = g_apPlanStatFns[param.wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nOperand = param.n;
    }

    if (pArgs->m_nArgCount == ARG_COUNT_WITHOUT_OPERAND)
    {
        pTarget = g_pGameEngine->m_ppSlots[(short)nOperand];
    }
    else
    {
        pTarget = g_pGameEngine->m_ppSlots[(short)pArgs->m_pArgs[ARG_OPERAND].n];
    }

    if ((nOperand & 0xffff) > SLOT_DROMER3)
    {
        // STRING: HYPERX 0x004bb348
        g_gameApp.Fatal("Fatal failure to initialize the receiver of a pass!");
    }

    if (pContext->GetActor()->GetSlot() == SLOT_ROK && pContext->GetActor()->GetPhase() == OBJECT_PHASE_LOCAL)
    {
        short nSlot = pContext->GetTargetSlot();

        AttachPak* const pTracked = pContext->GetActor()->GetPhysicsBody()->pAttach;

        if (pTracked != 0)
        {
            if (nSlot != pTracked->nSlot && nSlot != pContext->GetActor()->GetSlot())
            {
                return;
            }

            PlayerObject* const pOwner = g_pGameEngine->GetPlayer(pTracked->nSlot);

            pAxis->x = pTarget->GetPositionPtr()->x;
            pAxis->y = pTarget->GetPositionPtr()->y;
            pAxis->z = pTarget->GetPositionPtr()->z;

            Math::Vec3Normalize(pAxis);

            const float flDt = g_pGameEngine->m_clock.flDelta;

            *pSpeed = ((pOwner->m_stats.GetWeightedStat(RATING_SHOT_POWER) * 0.02f + 0.9f) * 100.0f) * flDt;

            EngineObject* const pActor = pContext->GetActor();

            PhysicsBody* const pBlock = pActor->GetPhysicsBody();

            Vec3::ScaleTo(pAxis, *pSpeed, &pBlock->vel);

            const int nBehaviour = pContext->m_nBehaviour;

            GameBehaviorObject* const pBehavior =
                pContext->GetActor()->GetPlanState()->GetBehaviorByIndex(nBehaviour);

            static_cast<CSteerToward*>(pBehavior->m_apSlots[0])->m_pTarget = pTarget;
        }
    }
    else
    {
        pAxis->x = pTarget->GetPositionPtr()->x;
        pAxis->y = pTarget->GetPositionPtr()->y;
        pAxis->z = pTarget->GetPositionPtr()->z;

        Math::Vec3Normalize(pAxis);

        *pSpeed = pContext->GetActor()->GetPhysicsBody()->GetForwardSpeedDt();

        EngineObject* const pActor = pContext->GetActor();

        PhysicsBody* const pBlock = pActor->GetPhysicsBody();

        Vec3::ScaleTo(pAxis, *pSpeed, &pBlock->vel);

        const int nBehaviour = pContext->m_nBehaviour;

        GameBehaviorObject* const pBehavior =
            pContext->GetActor()->GetPlanState()->GetBehaviorByIndex(nBehaviour);

        static_cast<CSteerToward*>(pBehavior->m_apSlots[0])->m_pTarget = pTarget;
    }

    if (*pSpeed <= g_flZeroPlanLaunch)
    {
        *pSpeed = SPEED_FLOOR;
    }
}
