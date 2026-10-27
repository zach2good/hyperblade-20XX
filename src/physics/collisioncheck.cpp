#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "engine/gameengine.h"
#include "physicsbody.h"
#include "planvm/planvm.h"
#include "common/pooledclasses.h"
#include "graphics/meshbone.h"
#include "globals.h"
#include "graphics/meshboundingbox.h"
#include "math/transformobject.h"
#include "math/vector.h"
#include "engine/devicector.h"
#include "planvm/plannode.h"
#include "boundbubble.h"
#include <math.h>
#include "app/gameapp.h"
#include "engine/engineobject.h"
#include "capsule.h"
#include "planvm/damagegate.h"
#include "engine/playerobject.h"
#include "physics/collisioncheck.h"
#include "engine/statids.h"
#include "engine/collisionobjectids.h"

// One and nine: the goal frame's thickness and the span between the outsides of the posts.
// GLOBAL: HYPERX 0x004b3364
extern const float g_flGoalSpanWidth;

// GLOBAL: HYPERX 0x004b3360
extern const float g_flGoalFrameWidth;

// Five, the goal mouth's height, and 400, the milliseconds a score is held against another.
// GLOBAL: HYPERX 0x004c2fe8
extern float g_flGoalZoneHeight;

// GLOBAL: HYPERX 0x004c2fec
extern const unsigned int g_nGoalScoreDebounceMs;

// One unit by ten for the acting object, one by four for its partner.
// GLOBAL: HYPERX 0x004c459c
extern float g_flMultiplierSelfRadius;

// GLOBAL: HYPERX 0x004c45a0
extern float g_flMultiplierSelfHeight;

// GLOBAL: HYPERX 0x004c45a4
extern float g_flMultiplierOtherRadius;

// GLOBAL: HYPERX 0x004c45a8
extern float g_flMultiplierOtherHeight;

// Half a unit by ten for the acting object, one by four for its partner.
// GLOBAL: HYPERX 0x004c4580
extern float g_flCapsuleSelfRadius;

// GLOBAL: HYPERX 0x004c4584
extern float g_flCapsuleSelfHeight;

// GLOBAL: HYPERX 0x004c4588
extern float g_flCapsuleOtherRadius;

// GLOBAL: HYPERX 0x004c458c
extern float g_flCapsuleOtherHeight;

// The box this check builds; the low y edge has its own value rather than half the height.
// GLOBAL: HYPERX 0x004c456c
extern float g_flVelBoxSizeX;
// GLOBAL: HYPERX 0x004c4570
extern float g_flVelBoxSizeZ;
// GLOBAL: HYPERX 0x004c4574
extern float g_flVelBoxSizeY;
// GLOBAL: HYPERX 0x004c4578
extern float g_flVelBoxDropY;

// Fifty. The speed that separates the two hit answers.
// GLOBAL: HYPERX 0x004c457c
extern float g_flVelHitSpeedGate;

// A post hit answers 1; a goal within 400 ms of the last answers 3, so it does not score twice.
// FUNCTION: HYPERX 0x004521d0
void __cdecl CollisionCheck::DromerLaserHurdleCollision(Interactor* pContext, PlanNode*, short* pResult, int)
{
    float flGate;
    // Never stored into the out buffer, only into the vector.
    Vec3            vLocal;
    Vec3            vScratch;
    Vec3            vDirOther;
    Vec3            vDirOwn;
    Vec3            vTrans;
    MeshBoundingBox boxTest;
    MeshBoundingBox boxPostB;
    MeshBoundingBox boxMouth;
    MeshBoundingBox boxPostA;
    MeshBoundingBox boxUpper;

    BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject* const pOther     = pColOther->m_pOwner;
    PhysicsBody* const  pOtherBody = pOther->GetPhysicsBody();

    BoundBubble* const pColOwn = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject* const pOwn     = pColOwn->m_pOwner;
    PhysicsBody* const  pOwnBody = pOwn->GetPhysicsBody();

    flGate = DamageGate();

    const float flDamage = ((PlayerObject*)pOwn)->m_stats.GetScalar(RATING_DAMAGE);

    if (flDamage <= flGate)
    {
        *pResult = 0;

        return;
    }

    float flDiffX = pOwnBody->pos.x - pOwnBody->prevPos.x;
    vScratch.y    = pOwnBody->pos.y - pOwnBody->prevPos.y;
    vScratch.z    = pOwnBody->pos.z - pOwnBody->prevPos.z;

    vLocal.x = flDiffX;
    vLocal.y = vScratch.y;
    vLocal.z = vScratch.z;

    TransformObject* const pTransform = &pOther->m_transformC;

    pTransform->TransformDirectionByRows(vLocal, &vScratch);

    vDirOwn = vScratch;

    flDiffX    = pOtherBody->pos.x - pOtherBody->prevPos.x;
    vScratch.y = pOtherBody->pos.y - pOtherBody->prevPos.y;
    vScratch.z = pOtherBody->pos.z - pOtherBody->prevPos.z;

    vLocal.x = flDiffX;
    vLocal.y = vScratch.y;
    vLocal.z = vScratch.z;

    pTransform->TransformDirectionByRows(vLocal, &vScratch);

    vDirOther = vScratch;

    const Vec3* const pCentre = pOwn->GetWorldPos(&vTrans);

    const float flTransX = pOther->m_transformC.m_a[0][3];

    vLocal.x = pCentre->x;
    vLocal.y = pCentre->y;
    vTrans.y = pOther->m_transformC.m_a[1][3];
    vLocal.z = pCentre->z;
    vTrans.z = pOther->m_transformC.m_a[2][3];

    vScratch.x = vLocal.x - flTransX;
    vScratch.y = vLocal.y - vTrans.y;
    vScratch.z = vLocal.z - vTrans.z;

    vLocal = vScratch;

    pTransform->TransformDirectionByRows(vLocal, &vLocal);

    boxTest.vMin.y = vLocal.y;
    boxTest.bValid = 1;
    boxTest.vMin.x = vLocal.x - 1.0f;

    boxPostA.vMin.y = 0.0f;
    boxPostA.bValid = 1;
    boxPostB.vMin.y = 0.0f;
    boxPostB.bValid = 1;

    boxTest.vMin.z = vLocal.z - 1.0f;

    boxTest.vMax.x = vLocal.x + 1.0f;
    boxTest.vMax.z = vLocal.z + 1.0f;
    boxTest.vMax.y = vLocal.y + 6.0f;

    float       flInner         = (g_flGoalSpanWidth - g_flGoalFrameWidth) * 0.5f;
    const float flPostHalfFrame = g_flGoalFrameWidth * 0.5f;

    boxPostA.vMin.x = flInner - flPostHalfFrame;
    boxPostA.vMin.z = -flPostHalfFrame;
    boxPostA.vMax.x = flPostHalfFrame + flInner;
    boxPostA.vMax.z = flPostHalfFrame;

    float flHeight = g_flGoalZoneHeight;

    boxPostA.vMax.y = flHeight;

    flInner = -flInner;

    boxPostB.vMax.z         = flPostHalfFrame;
    *(int*)&boxPostB.vMax.y = *(const int*)&flHeight;
    boxPostB.vMin.x         = flInner - flPostHalfFrame;
    boxPostB.vMin.z         = -flPostHalfFrame;
    boxPostB.vMax.x         = flPostHalfFrame + flInner;

    *pResult = 0;

    if (boxPostA.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest) != 0 ||
        boxPostB.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest) != 0)
    {
        *pResult = 1;

        return;
    }

    boxMouth.vMin.y = 0.0f;
    boxMouth.bValid = 1;

    const float flOuter = (g_flGoalSpanWidth - (g_flGoalFrameWidth + g_flGoalFrameWidth)) * 0.5f;

    boxMouth.vMin.x = -flOuter;

    float flMouthHalfFrame = g_flGoalFrameWidth * 0.5f;

    boxMouth.vMin.z         = -flMouthHalfFrame;
    *(int*)&boxMouth.vMax.z = *(const int*)&flMouthHalfFrame;
    boxMouth.vMax.x         = flOuter;
    const double dblHeight  = g_flGoalZoneHeight;
    boxMouth.vMax.y         = (float)dblHeight;

    if (boxMouth.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest) == 0)
    {
        return;
    }

    const float flTop      = g_flGoalZoneHeight - g_flGoalFrameWidth;
    const float flHalfSpan = g_flGoalSpanWidth * 0.5f;

    boxUpper.vMin.x = -flHalfSpan;

    boxUpper.bValid = 1;

    float flBarHalfFrame = g_flGoalFrameWidth * 0.5f;

    boxUpper.vMin.z         = -flBarHalfFrame;
    *(int*)&boxUpper.vMax.z = *(const int*)&flBarHalfFrame;
    boxUpper.vMin.y         = flTop;
    boxUpper.vMax.x         = flHalfSpan;
    boxUpper.vMax.y         = flTop + 5.0f;

    if (boxUpper.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest) == 0)
    {
        return;
    }

    const unsigned int nTick      = g_pGameEngine->m_clock.nTickMs;
    const unsigned int nLastScore = pOwn->m_nLastGoalTickMs;

    if (nLastScore != 0 && nTick - nLastScore < g_nGoalScoreDebounceMs)
    {
        *pResult = 3;

        return;
    }

    pOwn->m_nLastGoalTickMs = nTick;
    *pResult                = 2;
}

// id_DCF_DromerRampCollision, opcode 68: a miss answers two, a hit one above speed 50.
// FUNCTION: HYPERX 0x00469430
void __cdecl CollisionCheck::DromerRampCollision(Interactor* pContext, PlanNode*, short* pResult, int)
{
    int             nHit;
    Vec3            vScratch;
    Vec3            vLocal;
    Vec3            vDirOwn;
    Vec3            vDirOther;
    Vec3            vCentre;
    MeshBoundingBox boxScratch;
    Vec3            vDelta;
    MeshBoundingBox boxTest;
    MeshBoundingBox boxMesh;

    BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject*      pOther     = pColOther->m_pOwner;
    PhysicsBody* const pOtherBody = pOther->GetPhysicsBody();

    BoundBubble* const pColOwn = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject*      pOwn     = pColOwn->m_pOwner;
    PhysicsBody* const pOwnBody = pOwn->GetPhysicsBody();

    const Vec3* pCentre = pOwn->GetWorldPos(&vScratch);

    vCentre.x = pCentre->x;
    vCentre.y = pCentre->y;
    vCentre.z = pCentre->z;

    boxMesh = *pOther->m_pBones->GetBoundingBox(&boxScratch);

    float flDiffX = pOtherBody->pos.x - pOtherBody->prevPos.x;
    vLocal.y      = pOtherBody->pos.y - pOtherBody->prevPos.y;
    vLocal.z      = pOtherBody->pos.z - pOtherBody->prevPos.z;

    vScratch.x = flDiffX;
    vScratch.y = vLocal.y;
    vScratch.z = vLocal.z;

    TransformObject* const pTransform = &pOther->m_transformC;

    pTransform->TransformDirectionByRows(vScratch, &vScratch);

    vDirOther = vScratch;

    vScratch.x = pOther->m_transformC.m_a[0][3];
    vScratch.y = pOther->m_transformC.m_a[1][3];
    vScratch.z = pOther->m_transformC.m_a[2][3];

    Vec3* const pDelta = vCentre.Subtract(&vDelta, &vScratch);

    vLocal = *pDelta;

    pTransform->TransformDirectionByRows(*pDelta, &vLocal);

    const float flLowY = vLocal.y - g_flVelBoxDropY;

    vScratch.Set(vLocal.x, vLocal.y, vLocal.z);

    boxScratch.vMin.x       = vScratch.x - g_flVelBoxSizeX * 0.5f;
    boxScratch.bValid       = 1;
    const float flHalfDepth = g_flVelBoxSizeZ * 0.5f;
    boxScratch.vMin.z       = vScratch.z - flHalfDepth;
    boxScratch.vMin.y       = flLowY;
    boxScratch.vMax.x       = g_flVelBoxSizeX * 0.5f + vScratch.x;
    boxScratch.vMax.z       = flHalfDepth + vScratch.z;
    boxScratch.vMax.y       = g_flVelBoxSizeY + flLowY;

    boxTest = boxScratch;

    flDiffX  = pOwnBody->pos.x - pOwnBody->prevPos.x;
    vLocal.y = pOwnBody->pos.y - pOwnBody->prevPos.y;
    vLocal.z = pOwnBody->pos.z - pOwnBody->prevPos.z;

    vScratch.x = flDiffX;
    vScratch.y = vLocal.y;
    vScratch.z = vLocal.z;

    pTransform->TransformDirectionByRows(vScratch, &vScratch);

    vDirOwn = vScratch;

    nHit = boxMesh.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest);

    PlanState* const pPlan = pOwn->GetPlanState();

    if (nHit != 0 && pPlan->m_pCurrent->m_wKind != BEH_FLY)
    {
        const Vec3* pVelocity = &pOwn->m_pBody->vel;

        vLocal = *pVelocity;

        if ((float)(sqrt(vLocal.x * vLocal.x + (vLocal.y * vLocal.y + vLocal.z * vLocal.z)) * g_pGameEngine->m_clock.flInvDelta) > g_flVelHitSpeedGate)
        {
            *pResult = 1;

            return;
        }

        *pResult = 0;

        return;
    }

    *pResult = 2;
}

// One, nine and ten: the goal frame's thickness, the span between the posts, the mouth height.
// GLOBAL: HYPERX 0x004c4590
extern float g_flZoneFrameWidth;
// GLOBAL: HYPERX 0x004c4594
extern float g_flZoneSpanWidth;
// GLOBAL: HYPERX 0x004c4598
extern float g_flZoneHeight;

// Swept capsules in the acting object's frame; a hit answers nought and a miss one.
// FUNCTION: HYPERX 0x004697b0
void __cdecl CollisionCheck::DromerRandomizerCollision(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    const short nSlot = (short)nArg;

    EngineObject* pOther;
    EngineObject* pOwn;
    Vec3          vLocal;
    Vec3          vCentre;
    Vec3          vOut;
    Vec3          vScratch;
    Vec3          vDirOwn;
    Vec3          vDirOther;
    Capsule       capOwn;
    Capsule       capOther;

    if (pContext->GetNamedSlot() == -1)
    {
        BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pOther = pColOther->m_pOwner;

        BoundBubble* const pColOwn = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

        pOwn = pColOwn->m_pOwner;
    }
    else
    {
        pOther = pContext->GetActor();
        pOwn   = g_pGameEngine->m_ppSlots[nSlot];
    }

    PhysicsBody* const pOtherBody = pOther->GetPhysicsBody();
    PhysicsBody* const pOwnBody   = pOwn->GetPhysicsBody();

    const Vec3* pCentre = pOwn->GetWorldPos(&vScratch);

    vCentre.x = pCentre->x;
    vCentre.y = pCentre->y;
    vCentre.z = pCentre->z;

    float flDiffX = pOtherBody->pos.x - pOtherBody->prevPos.x;
    vScratch.y    = pOtherBody->pos.y - pOtherBody->prevPos.y;
    vScratch.z    = pOtherBody->pos.z - pOtherBody->prevPos.z;

    vLocal.x = flDiffX;
    vLocal.y = vScratch.y;
    vLocal.z = vScratch.z;

    TransformObject* const pTransform = &pOther->m_transformC;

    pTransform->TransformDirectionByRows(vLocal, &vScratch);

    vDirOther = vScratch;

    flDiffX = pOwnBody->pos.x - pOwnBody->prevPos.x;
    vOut.y  = pOwnBody->pos.y - pOwnBody->prevPos.y;
    vOut.z  = pOwnBody->pos.z - pOwnBody->prevPos.z;

    vLocal.x = flDiffX;
    vLocal.y = vOut.y;
    vLocal.z = vOut.z;

    vScratch.x = 0.0f;
    vScratch.y = 0.0f;
    vScratch.z = 0.0f;

    pTransform->TransformDirectionByRows(vLocal, &vOut);

    vDirOwn = vOut;

    const float flTransX = pOther->m_transformC.m_a[0][3];
    vLocal.y             = pOther->m_transformC.GetElement(1, 3);
    vLocal.z             = pOther->m_transformC.m_a[2][3];

    vOut.x = vCentre.x - flTransX;
    vOut.y = vCentre.y - vLocal.y;
    vOut.z = vCentre.z - vLocal.z;

    vLocal = vOut;

    pTransform->TransformDirectionByRows(vLocal, &vLocal);

    capOther.bValid   = 1;
    capOther.vCentre  = vScratch;
    capOther.flRadius = g_flCapsuleSelfRadius;
    capOther.flHeight = g_flCapsuleSelfHeight;

    capOwn.bValid   = 1;
    capOwn.vCentre  = vLocal;
    capOwn.flRadius = g_flCapsuleOtherRadius;
    capOwn.flHeight = g_flCapsuleOtherHeight;

    const int nHit = capOther.CheckCollision(&vDirOther, &vDirOwn, &capOwn);

    *pResult = !nHit;
}

// id_DCF_DromerLimboCollision, opcode 65: a goal's posts and bar answer nought, its mouth two.
// FUNCTION: HYPERX 0x00469a70
void __cdecl CollisionCheck::DromerLimboCollision(Interactor* pContext, PlanNode*, short* pResult, int)
{
    int             nHitPostA;
    int             nHitPostB;
    int             nHitBar;
    int             nHitMouth;
    Vec3            vLocal;
    Vec3            vScratch;
    Vec3            vDirOther;
    Vec3            vDirOwn;
    MeshBoundingBox boxTest;
    Vec3            vCentre;
    MeshBoundingBox boxPostA;
    MeshBoundingBox boxPostB;
    MeshBoundingBox boxMouth;
    MeshBoundingBox boxBar;
    Vec3            vDelta;

    BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject*      pOther     = pColOther->m_pOwner;
    PhysicsBody* const pOtherBody = pOther->GetPhysicsBody();

    BoundBubble* const pColOwn = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject* const pOwn     = pColOwn->m_pOwner;
    PhysicsBody* const  pOwnBody = pOwn->GetPhysicsBody();

    const Vec3* pCentre = pOwn->GetWorldPos(&vScratch);

    vCentre.x = pCentre->x;
    vCentre.y = pCentre->y;
    vCentre.z = pCentre->z;

    float flDiffX = pOwnBody->pos.x - pOwnBody->prevPos.x;
    vScratch.y    = pOwnBody->pos.y - pOwnBody->prevPos.y;
    vScratch.z    = pOwnBody->pos.z - pOwnBody->prevPos.z;

    vLocal.x = flDiffX;
    vLocal.y = vScratch.y;
    vLocal.z = vScratch.z;

    TransformObject* const pTransform = &pOther->m_transformC;

    pTransform->TransformDirectionByRows(vLocal, &vScratch);

    vDirOwn = vScratch;

    vScratch.x = pOther->m_transformC.m_a[0][3];
    vScratch.y = pOther->m_transformC.m_a[1][3];
    vScratch.z = pOther->m_transformC.m_a[2][3];

    Vec3* const pDelta = vCentre.Subtract(&vDelta, &vScratch);

    vLocal = *pDelta;

    pTransform->TransformDirectionByRows(*pDelta, &vLocal);

    const float flRadius     = pColOwn->m_flRadius;
    const float flHalfRadius = flRadius * 0.5f;

    boxTest.vMin.y = vLocal.y;
    boxTest.bValid = 1;
    boxTest.vMin.x = vLocal.x - flHalfRadius * 0.5f;
    boxTest.vMin.z = vLocal.z - flHalfRadius * 0.5f;
    boxTest.vMax.x = flHalfRadius * 0.5f + vLocal.x;
    boxTest.vMax.z = flHalfRadius * 0.5f + vLocal.z;
    boxTest.vMax.y = flRadius + (flRadius + vLocal.y);

    flDiffX    = pOtherBody->pos.x - pOtherBody->prevPos.x;
    vScratch.y = pOtherBody->pos.y - pOtherBody->prevPos.y;
    vScratch.z = pOtherBody->pos.z - pOtherBody->prevPos.z;

    vLocal.x = flDiffX;
    vLocal.y = vScratch.y;
    vLocal.z = vScratch.z;

    pTransform->TransformDirectionByRows(vLocal, &vScratch);

    const float flHalfSpan = g_flZoneSpanWidth * 0.5f;

    vDirOther = vScratch;

    boxBar.vMin.x = -flHalfSpan;

    const float flHalfFrame = g_flZoneFrameWidth * 0.5f;

    boxBar.vMin.y = g_flZoneHeight;
    boxBar.bValid = 1;

    const float flNegHalfFrame = -flHalfFrame;

    boxBar.vMin.z = flNegHalfFrame;
    boxBar.vMax.x = flHalfSpan;
    boxBar.vMax.z = flHalfFrame;
    float flTop   = g_flZoneFrameWidth;
    flTop += g_flZoneHeight;
    boxBar.vMax.y = flTop;

    const float flInner = (g_flZoneSpanWidth - g_flZoneFrameWidth) * 0.5f;

    boxPostA.vMin.y = 0.0f;
    boxPostA.vMin.z = flNegHalfFrame;
    boxPostA.vMin.x = flInner - flHalfFrame;
    boxPostA.vMax.x = flHalfFrame + flInner;
    boxPostA.bValid = 1;

    boxPostB.vMin.y = 0.0f;
    boxPostB.bValid = 1;
    boxPostB.vMin.z = flNegHalfFrame;

    boxMouth.vMin.z = -(1.0f / 24.0f);
    boxMouth.vMin.y = 0.0f;
    boxMouth.vMax.z = 1.0f / 24.0f;
    boxMouth.bValid = 1;

    boxPostA.vMax.z = flHalfFrame;

    const float flHeight = g_flZoneHeight;

    boxPostA.vMax.y = flHeight;
    boxPostB.vMax.y = flHeight;
    boxPostB.vMin.x = -flInner - flHalfFrame;
    boxPostB.vMax.x = flHalfFrame + -flInner;
    boxPostB.vMax.z = flHalfFrame;

    const float flOuter = (g_flZoneSpanWidth - (g_flZoneFrameWidth + g_flZoneFrameWidth)) * 0.5f;

    boxMouth.vMin.x = -flOuter;
    boxMouth.vMax.x = flOuter;
    boxMouth.vMax.y = flHeight;

    nHitPostA = boxPostA.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest);
    nHitPostB = boxPostB.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest);
    nHitBar   = boxBar.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest);
    nHitMouth = boxMouth.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest);

    const int bFrameHit = nHitPostA != 0 || nHitPostB != 0 || nHitBar != 0;

    if (bFrameHit != 0)
    {
        *pResult = 0;

        return;
    }

    if (nHitMouth != 0)
    {
        *pResult = 2;
    }
    else
    {
        *pResult = 1;
    }
}

// id_DCF_BouncedInGoal in DATA/ENUMIDS.H: the plan opcode table registers this body at 124.
// FUNCTION: HYPERX 0x00469ef0
void __cdecl CollisionCheck::BouncedInGoal(Interactor*, PlanNode*, short* pResult, int)
{
    BoundBubble* const pRok = g_pGameEngine->GetColObjPtr(COB_ROK);

    BoundBubble* const pNorthPort = g_pGameEngine->GetColObjPtr(COB_NGOAL_PORT);

    BoundBubble* const pSouthPort = g_pGameEngine->GetColObjPtr(COB_SGOAL_PORT);

    if (pRok->DoesIntersect(pNorthPort) != 0)
    {
        *pResult = 1;

        return;
    }

    if (pRok->DoesIntersect(pSouthPort) != 0)
    {
        *pResult = 2;
    }
    else
    {
        *pResult = 0;
    }
}

// The box the test is built around, one dimension per axis.
// GLOBAL: HYPERX 0x004c45ac
extern float g_flColBoxSizeX;
// GLOBAL: HYPERX 0x004c45b0
extern float g_flColBoxSizeZ;
// GLOBAL: HYPERX 0x004c45b4
extern float g_flColBoxSizeY;

// 0x004697b0 with the acting object's capsule one unit thick rather than half.
// FUNCTION: HYPERX 0x00469f90
void __cdecl CollisionCheck::DromerMultiplierCollision(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    const short nSlot = (short)nArg;

    EngineObject* pOther;
    EngineObject* pOwn;
    Vec3          vLocal;
    Vec3          vCentre;
    Vec3          vOut;
    Vec3          vScratch;
    Vec3          vDirOwn;
    Vec3          vDirOther;
    Capsule       capOwn;
    Capsule       capOther;

    if (pContext->GetNamedSlot() == -1)
    {
        BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pOther = pColOther->m_pOwner;

        BoundBubble* const pColOwn = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

        pOwn = pColOwn->m_pOwner;
    }
    else
    {
        pOther = pContext->GetActor();
        pOwn   = g_pGameEngine->m_ppSlots[nSlot];
    }

    PhysicsBody* const pOtherBody = pOther->GetPhysicsBody();
    PhysicsBody* const pOwnBody   = pOwn->GetPhysicsBody();

    const Vec3* pCentre = pOwn->GetWorldPos(&vScratch);

    vCentre.x = pCentre->x;
    vCentre.y = pCentre->y;
    vCentre.z = pCentre->z;

    float flDiffX = pOtherBody->pos.x - pOtherBody->prevPos.x;
    vScratch.y    = pOtherBody->pos.y - pOtherBody->prevPos.y;
    vScratch.z    = pOtherBody->pos.z - pOtherBody->prevPos.z;

    vLocal.x = flDiffX;
    vLocal.y = vScratch.y;
    vLocal.z = vScratch.z;

    TransformObject* const pTransform = &pOther->m_transformC;

    pTransform->TransformDirectionByRows(vLocal, &vScratch);

    vDirOther = vScratch;

    flDiffX = pOwnBody->pos.x - pOwnBody->prevPos.x;
    vOut.y  = pOwnBody->pos.y - pOwnBody->prevPos.y;
    vOut.z  = pOwnBody->pos.z - pOwnBody->prevPos.z;

    vLocal.x = flDiffX;
    vLocal.y = vOut.y;
    vLocal.z = vOut.z;

    vScratch.x = 0.0f;
    vScratch.y = 0.0f;
    vScratch.z = 0.0f;

    pTransform->TransformDirectionByRows(vLocal, &vOut);

    vDirOwn = vOut;

    const float flTransX = pOther->m_transformC.m_a[0][3];
    vLocal.y             = pOther->m_transformC.GetElement(1, 3);
    vLocal.z             = pOther->m_transformC.m_a[2][3];

    vOut.x = vCentre.x - flTransX;
    vOut.y = vCentre.y - vLocal.y;
    vOut.z = vCentre.z - vLocal.z;

    vLocal = vOut;

    pTransform->TransformDirectionByRows(vLocal, &vLocal);

    capOther.bValid   = 1;
    capOther.vCentre  = vScratch;
    capOther.flRadius = g_flMultiplierSelfRadius;
    capOther.flHeight = g_flMultiplierSelfHeight;

    capOwn.bValid   = 1;
    capOwn.vCentre  = vLocal;
    capOwn.flRadius = g_flMultiplierOtherRadius;
    capOwn.flHeight = g_flMultiplierOtherHeight;

    const int nHit = capOther.CheckCollision(&vDirOther, &vDirOwn, &capOwn);

    *pResult = !nHit;
}

// id_DCF_DromerHurdleCollision, opcode 61: slot eight against slot seven's mesh; a hit is nought.
// FUNCTION: HYPERX 0x0046a250
void __cdecl CollisionCheck::DromerHurdleCollision(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3            vScratch;
    Vec3            vLocal;
    Vec3            vDirOwn;
    Vec3            vDirOther;
    Vec3            vCentre;
    MeshBoundingBox boxTest;
    Vec3            vDelta;
    MeshBoundingBox boxRaw;

    BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject*      pOther     = pColOther->m_pOwner;
    PhysicsBody* const pOtherBody = pOther->GetPhysicsBody();

    const short        nOwnSlot = pContext->GetModelSlot();
    BoundBubble* const pColOwn  = g_pGameEngine->GetColObjPtr(nOwnSlot);

    EngineObject* const pOwn     = pColOwn->m_pOwner;
    PhysicsBody* const  pOwnBody = pOwn->GetPhysicsBody();

    const Vec3* pCentre = pOwn->GetWorldPos(&vScratch);

    vCentre.x = pCentre->x;
    vCentre.y = pCentre->y;
    vCentre.z = pCentre->z;

    vScratch.x = pOther->m_transformC.m_a[0][3];
    vScratch.y = pOther->m_transformC.m_a[1][3];
    vScratch.z = pOther->m_transformC.m_a[2][3];

    Vec3* const pDelta = vCentre.Subtract(&vDelta, &vScratch);

    vLocal = *pDelta;

    TransformObject* const pTransform = &pOther->m_transformC;

    pTransform->TransformDirectionByRows(*pDelta, &vLocal);

    vScratch = vLocal;

    const float flLowY      = vLocal.y - 2.0f;
    boxTest.vMin.x          = vScratch.x - g_flColBoxSizeX * 0.5f;
    boxTest.bValid          = 1;
    const float flHalfDepth = g_flColBoxSizeZ * 0.5f;
    boxTest.vMin.z          = vScratch.z - flHalfDepth;
    boxTest.vMin.y          = flLowY;
    boxTest.vMax.x          = g_flColBoxSizeX * 0.5f + vScratch.x;
    boxTest.vMax.z          = flHalfDepth + vScratch.z;
    boxTest.vMax.y          = g_flColBoxSizeY + flLowY;

    float flDiffX = pOwnBody->pos.x - pOwnBody->prevPos.x;
    vLocal.y      = pOwnBody->pos.y - pOwnBody->prevPos.y;
    vLocal.z      = pOwnBody->pos.z - pOwnBody->prevPos.z;

    vScratch.x = flDiffX;
    vScratch.y = vLocal.y;
    vScratch.z = vLocal.z;

    pTransform->TransformDirectionByRows(vScratch, &vScratch);

    vDirOwn = vScratch;

    MeshBoundingBox boxMesh = *pOther->m_pBones->GetBoundingBox(&boxRaw);

    flDiffX  = pOtherBody->pos.x - pOtherBody->prevPos.x;
    vLocal.y = pOtherBody->pos.y - pOtherBody->prevPos.y;
    vLocal.z = pOtherBody->pos.z - pOtherBody->prevPos.z;

    vScratch.x = flDiffX;
    vScratch.y = vLocal.y;
    vScratch.z = vLocal.z;

    pTransform->TransformDirectionByRows(vScratch, &vScratch);

    vDirOther = vScratch;

    if (boxMesh.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest) != 0)
    {
        PlanState* const pDiscarded = pOwn->GetPlanState();

        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// The two radius divisors, and the zone box's width, height and depth.
// GLOBAL: HYPERX 0x004c45b8
extern float g_flSplitBoxXDiv;
// GLOBAL: HYPERX 0x004c45bc
extern float g_flSplitBoxZDiv;
// GLOBAL: HYPERX 0x004c45c0
extern float g_flSplitZoneDepth;
// GLOBAL: HYPERX 0x004c45c4
extern float g_flSplitZoneHeight;
// GLOBAL: HYPERX 0x004c45c8
extern float g_flSplitZoneWidth;

namespace CollisionCheck
{

static inline void SetBoxCorners(MeshBoundingBox* pBox, const Vec3* pMin, const Vec3* pMax);

}

inline void CollisionCheck::SetBoxCorners(MeshBoundingBox* pBox, const Vec3* pMin, const Vec3* pMax)
{
    pBox->vMin   = *pMin;
    pBox->vMax   = *pMax;
    pBox->bValid = 1;
}

// id_DCF_RokHurdleCollision, opcode 63: the box's y extent comes from the local z.
// FUNCTION: HYPERX 0x0046a540
void __cdecl CollisionCheck::RokHurdleCollision(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3            vLocal;
    Vec3            vCentre;
    Vec3            vTrans;
    Vec3            vDelta;
    Vec3            vDirOther;
    Vec3            vDirOwn;
    MeshBoundingBox boxTest;
    MeshBoundingBox boxZone;

    BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject*      pOther     = pColOther->m_pOwner;
    PhysicsBody* const pOtherBody = pOther->GetPhysicsBody();

    BoundBubble* const pColOwn = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject* const pOwn     = pColOwn->m_pOwner;
    PhysicsBody* const  pOwnBody = pOwn->GetPhysicsBody();

    const Vec3* pCentre = pOwn->GetWorldPos(&vTrans);

    vCentre.x = pCentre->x;
    vCentre.y = pCentre->y;
    vCentre.z = pCentre->z;

    vTrans.x = pOther->m_transformC.m_a[0][3];
    vTrans.y = pOther->m_transformC.m_a[1][3];
    vTrans.z = pOther->m_transformC.m_a[2][3];

    Vec3* const pDelta = vCentre.Subtract(&vDelta, &vTrans);

    vLocal = *pDelta;

    TransformObject* const pTransform = &pOther->m_transformC;

    pTransform->TransformDirectionByRows(*pDelta, &vLocal);

    const float flRadius = pColOwn->m_flRadius;

    vDelta.x = vLocal.x - flRadius / g_flSplitBoxXDiv;
    vDelta.z = vLocal.z - flRadius / g_flSplitBoxZDiv;
    vDelta.y = vLocal.z - flRadius;
    vTrans.x = flRadius / g_flSplitBoxXDiv + vLocal.x;
    vTrans.z = flRadius / g_flSplitBoxZDiv + vLocal.z;
    vTrans.y = vLocal.z + flRadius;

    SetBoxCorners(&boxTest, &vDelta, &vTrans);

    const float flDiffX = pOwnBody->pos.x - pOwnBody->prevPos.x;
    vDelta.y            = pOwnBody->pos.y - pOwnBody->prevPos.y;
    vDelta.z            = pOwnBody->pos.z - pOwnBody->prevPos.z;

    vTrans.x = flDiffX;
    vTrans.y = vDelta.y;
    vTrans.z = vDelta.z;

    pTransform->TransformDirectionByRows(vTrans, &vTrans);

    vDirOwn = vTrans;

    vTrans.x = g_flSplitZoneWidth * -0.5f;
    vTrans.y = 0.0f;
    vTrans.z = g_flSplitZoneDepth * -0.5f;
    vDelta.y = g_flSplitZoneHeight;
    vDelta.x = g_flSplitZoneWidth * 0.5f;
    vDelta.z = g_flSplitZoneDepth * 0.5f;

    SetBoxCorners(&boxZone, &vTrans, &vDelta);

    const float flDx = pOtherBody->pos.x - pOtherBody->prevPos.x;
    const float flDy = pOtherBody->pos.y - pOtherBody->prevPos.y;
    const float flDz = pOtherBody->pos.z - pOtherBody->prevPos.z;

    vTrans.x = flDx * pTransform->m_a[0][0] + flDy * pTransform->m_a[1][0] + flDz * pTransform->m_a[2][0];
    vTrans.y = flDx * pTransform->m_a[0][1] + flDy * pTransform->m_a[1][1] + flDz * pTransform->m_a[2][1];
    vTrans.z = flDx * pTransform->m_a[0][2] + flDy * pTransform->m_a[1][2] + flDz * pTransform->m_a[2][2];

    vDirOther = vTrans;

    const int nHit = boxZone.StepIntersectTest(&vDirOther, &vDirOwn, &boxTest);

    *pResult = !nHit;
}

// Y is always zero, so the push-out never moves an object vertically.
// FUNCTION: HYPERX 0x0046a890
Vec3* __cdecl CollisionCheck::ComputeNearestNormal(Vec3* pNormalOut, const MeshBoundingBox* pBox, Vec3* pPoint)
{
    Vec3  vNormal;
    Vec3  vPoint;
    float flScale;

    if (pBox->vMin.x > pPoint->x || pPoint->x > pBox->vMax.x)
    {
        if (pBox->vMin.z > pPoint->z || pPoint->z > pBox->vMax.z)
        {
            vPoint.x = pPoint->x;
            vPoint.y = pPoint->y;
            vPoint.z = pPoint->z;

            vNormal = vPoint;

            flScale = 1.0f / (float)sqrt((g_flZeroPlan * g_flZeroPlan + vPoint.z * vPoint.z) + vPoint.x * vPoint.x);

            vNormal.x = flScale;
            vNormal.x *= vPoint.x;
            vNormal.y = g_flZeroPlan * flScale;
            float flZ = flScale;
            flZ *= vNormal.z;
            vNormal.z = flZ;
        }
        else if (pBox->vMax.x <= pPoint->x)
        {
            vNormal.x = 1.0f;
            vNormal.y = g_flZeroPlan;
            vNormal.z = g_flZeroPlan;
        }
        else
        {
            vNormal.x = -1.0f;
            vNormal.y = g_flZeroPlan;
            vNormal.z = g_flZeroPlan;
        }
    }
    else if (pBox->vMax.z <= pPoint->z)
    {
        vNormal.z = 1.0f;
        vNormal.x = g_flZeroPlan;
        vNormal.y = g_flZeroPlan;
    }
    else
    {
        vNormal.z = -1.0f;
        vNormal.x = g_flZeroPlan;
        vNormal.y = g_flZeroPlan;
    }

    *pNormalOut = vNormal;

    return pNormalOut;
}

extern float g_flGoalZoneHeight;
float        g_flGoalZoneHeight = 5.0f;

extern const unsigned int g_nGoalScoreDebounceMs;
const unsigned int        g_nGoalScoreDebounceMs = 400;
float                     g_flVelBoxSizeX        = 1.0f;
float                     g_flVelBoxSizeZ        = 1.0f;
float                     g_flVelBoxSizeY        = 4.0f;
float                     g_flVelBoxDropY        = 4.0f;
float                     g_flVelHitSpeedGate    = 50.0f;

extern float g_flCapsuleSelfRadius;
float        g_flCapsuleSelfRadius = 0.5f;

extern float g_flCapsuleSelfHeight;
float        g_flCapsuleSelfHeight = 10.0f;

extern float g_flCapsuleOtherRadius;
float        g_flCapsuleOtherRadius = 1.0f;

extern float g_flCapsuleOtherHeight;
float        g_flCapsuleOtherHeight = 4.0f;
float        g_flZoneFrameWidth     = 1.0f;
float        g_flZoneSpanWidth      = 9.0f;
float        g_flZoneHeight         = 10.0f;

extern float g_flMultiplierSelfRadius;
float        g_flMultiplierSelfRadius = 1.0f;

extern float g_flMultiplierSelfHeight;
float        g_flMultiplierSelfHeight = 10.0f;

extern float g_flMultiplierOtherRadius;
float        g_flMultiplierOtherRadius = 1.0f;

extern float g_flMultiplierOtherHeight;
float        g_flMultiplierOtherHeight = 4.0f;
float        g_flColBoxSizeX           = 1.0f;
float        g_flColBoxSizeZ           = 1.0f;
float        g_flColBoxSizeY           = 4.0f;

extern float g_flSplitBoxXDiv;
float        g_flSplitBoxXDiv = 1.0f;

extern float g_flSplitBoxZDiv;
float        g_flSplitBoxZDiv = 1.0f;

extern float g_flSplitZoneDepth;
float        g_flSplitZoneDepth = 3.0f;

extern float g_flSplitZoneHeight;
float        g_flSplitZoneHeight = 5.0f;

extern float g_flSplitZoneWidth;
float        g_flSplitZoneWidth = 9.0f;
