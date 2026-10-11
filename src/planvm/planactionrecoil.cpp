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
#include "engine/crelaxyaw.h"
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

// The impulse is the struck object's velocity scaled, plus independent scatter on each axis.

// The guards are arity: a count equal to N means argument N is absent.
#define ARG_FIRST  1
#define ARG_SECOND 2

// A context from a collision pair names no slot.
#define SLOT_EXPLICIT_PAIR (-1)

// Three, the facing step before the cross product decides whether to mirror it.
// GLOBAL: HYPERX 0x004b1528
extern const float g_flFacingStepBase;

// Seven animation ids per speed step; src/engine/object.cpp marks 0x004cf9fc.
extern short g_awReactionByDamageAndSector[35];

// The two steps the table is indexed by, published before they are used.
// GLOBAL: HYPERX 0x004d2b80
extern short g_nCheckImpulseStep;
// GLOBAL: HYPERX 0x004d2b82
extern short g_nCheckImpulseFacing;

// GLOBAL: HYPERX 0x004cb384
extern short g_nCheckImpulseOtherSlot;
// GLOBAL: HYPERX 0x004cb386
extern short g_nCheckImpulseSelfSlot;

// FUNCTION: HYPERX 0x00404d20
void __cdecl PlanAction::RecoilOffWall(Interactor* pContext, PlanNode* pNode, short*, int nArg)
{
    BoundBubble*  pColOther;
    EngineObject* pSelf;
    EngineObject* pOther;
    EngineObject* pAnnounce;
    Vec3          vImpulse;
    Vec3          vDiff;
    float         flFactor;
    float         flMag;

    if (pContext->GetNamedSlot() == SLOT_EXPLICIT_PAIR)
    {
        BoundBubble* const pColSelf = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

        pSelf = pColSelf->GetOwner();

        pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pOther = pColOther->GetOwner();

        if (pNode->m_nArgCount == 1)
        {
            flFactor = -0.5f;
        }
        else
        {
            flFactor = (float)pNode->m_pArgs[ARG_FIRST].n * 0.01f;
        }
    }
    else
    {
        pOther = pContext->GetActor();

        PlanWord* const pArgs = pNode->m_pArgs;
        int             nSlot;

        if (pArgs[ARG_FIRST].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nSlot = (short)nArg;
        }
        else
        {
            nSlot = (short)pArgs[ARG_FIRST].n;
        }

        pSelf = g_pGameEngine->m_ppSlots[nSlot];

        if (pNode->m_nArgCount == 2)
        {
            flFactor = -0.5f;
        }
        else
        {
            flFactor = (float)pArgs[ARG_SECOND].n * 0.01f;
        }
    }

    PhysicsBody* pBody = pSelf->GetPhysicsBody();

    const float flRatio = pBody->flSpeed / pBody->GetForwardSpeedDt() * 4.0f;

    Vec3::ScaleTo(&pBody->vel, flFactor, &vImpulse);

    flMag = (float)sqrt(Vec3::DotInline(&vImpulse, &vImpulse)) * 0.125f;

    vImpulse.x = ((float)rand() * (1.0f / RAND_MAX) - 0.5f) * flMag + vImpulse.x;
    vImpulse.y = ((float)rand() * (1.0f / RAND_MAX) - 0.5f) * flMag + vImpulse.y;
    vImpulse.z = ((float)rand() * (1.0f / RAND_MAX) - 0.5f) * flMag + vImpulse.z;

    pSelf->SetVelocity(vImpulse);

    if (pSelf->GetSlot() < PLAYER_OBJECT_COUNT)
    {
        CRelaxYaw* const pRelax = (CRelaxYaw*)AllocAgendaItem(Agenda_CRelaxYaw);

        GameBehaviorObject* const pBindNode = pSelf->GetPlanState()->m_pCurrent;
        pRelax->Bind(pSelf, pBindNode);
        pRelax->m_flAngle = g_flHitSpin;
        pRelax->Activate();

        float flFacing = g_flFacingStepBase;

        Vec3 aScratch[2];

        const Vec3* pOtherX = pOther->GetWorldPos(&aScratch[0]);
        const Vec3* pSelfX  = pSelf->GetWorldPos(&aScratch[1]);
        vDiff.x             = pOtherX->x - pSelfX->x;

        const Vec3* pOtherY = pOther->GetWorldPos(&aScratch[1]);
        const Vec3* pSelfY  = pSelf->GetWorldPos(&aScratch[0]);
        vDiff.y             = pOtherY->y - pSelfY->y;

        const Vec3* pOtherZ = pOther->GetWorldPos(&aScratch[1]);
        const Vec3* pSelfZ  = pSelf->GetWorldPos(&aScratch[0]);
        vDiff.z             = pOtherZ->z - pSelfZ->z;

        Math::Vec3Normalize(&vDiff);

        Vec3* const pDir = &pBody->velDir;

        vImpulse.x = vDiff.z * pDir->y - vDiff.y * pDir->z;
        vImpulse.y = vDiff.x * pDir->z - vDiff.z * pDir->x;
        vImpulse.z = vDiff.y * pDir->x - vDiff.x * pDir->y;

        if (vImpulse.z <= 0.0)
        {
            if (flFacing > 0.0f)
            {
                flFacing = 7.0f - flFacing;
            }
            else
            {
                flFacing = 0.0f;
            }
        }

        if (flRatio > 4.0f)
        {
            g_nCheckImpulseStep = 4;
        }
        else
        {
            g_nCheckImpulseStep = (short)flRatio;
        }

        g_nCheckImpulseFacing = (short)flFacing;

        pSelf->PostSceneEvent(MSG_SET_ANIMATION,
                              g_awReactionByDamageAndSector[g_nCheckImpulseStep * 7 + g_nCheckImpulseFacing]);
        pSelf->PostSceneEvent(MSG_YER_HIT, 0);

        const short nViewed = g_pGameEngine->m_nViewedSlot;

        if (pSelf->GetSlot() == nViewed)
        {
            pAnnounce = g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN];

            pAnnounce->PostSceneEvent(MSG_HIT, pSelf->GetSlot());

            g_nCheckImpulseSelfSlot  = pSelf->GetSlot();
            g_nCheckImpulseOtherSlot = pOther->GetSlot();
        }
    }
}

// Action 76, a partial twin of ReflectOffModel: box geometry instead of the shape dispatch.

// Five, the box's top.
// GLOBAL: HYPERX 0x004b1548
extern const float g_flWallBoxTopY;

// FUNCTION: HYPERX 0x004050d0
void __cdecl PlanAction::ReflectOffWall(Interactor* pContext, PlanNode* pNode, short*, int nArg)
{
    const short nSlot = (short)nArg;

    BoundBubble*    pColOther;
    Vec3*           pNormalField;
    EngineObject*   pSelf;
    EngineObject*   pOther;
    short           nResolved;
    MeshBoundingBox box;
    Vec3            vSelfPos;
    Vec3            vOtherPos;
    Vec3            vPoint;
    Vec3            vOrigin;
    Vec3            vDelta;
    float           flFactor;
    Vec3            vScratch;
    float           flSpeed;

    if (pNode->m_nArgCount < 3)
    {
        BoundBubble* const pColSelf = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

        pSelf = pColSelf->GetOwner();

        pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pOther = pColOther->GetOwner();

        if (pNode->m_nArgCount == 2)
        {
            flFactor = (float)pNode->m_pArgs[ARG_FIRST].n * 0.01f;
        }
        else
        {
            flFactor = -0.5f;
        }
    }
    else
    {
        PlanWord* const pArgs = pNode->m_pArgs;

        if (pArgs[ARG_SECOND].n != PLAN_TOKEN_MESSAGE_DATA)
        {
            return;
        }

        const short nOther = (short)pArgs[ARG_FIRST].n;

        pSelf = g_pGameEngine->m_ppSlots[nSlot];

        // Dead for a short; the client tests the short at 0x004051aa and indexes it at 0x004051c2.
        if ((unsigned short)((unsigned int)nOther >> 16) == PLAN_ARG_SELECTOR)
        {
            nResolved = g_apPlanStatFns[nOther & 0xffff](pContext->GetActor(), pNode);
        }
        else
        {
            nResolved = nOther;
        }

        flFactor = -0.03f;

        pOther = g_pGameEngine->m_ppSlots[nResolved];
    }

    PhysicsBody*       pSelfBody  = pSelf->GetPhysicsBody();
    PhysicsBody* const pOtherBody = pOther->GetPhysicsBody();

    vSelfPos = *pSelf->GetWorldPos(&vDelta);

    vOtherPos = *pOther->GetWorldPos(&vDelta);

    vOrigin.x = pOther->m_transformC.m_a[0][3];
    vOrigin.y = pOther->m_transformC.m_a[1][3];
    vOrigin.z = pOther->m_transformC.m_a[2][3];

    Vec3* const pDelta = vSelfPos.Subtract(&vDelta, &vOrigin);

    Vec3 vLocal = *pDelta;

    pOther->m_transformC.TransformDirectionByRows(*pDelta, &vLocal);

    vPoint = vLocal;

    vLocal.x  = g_flWallBoxSizeX * -0.5f;
    vOrigin.x = g_flWallBoxSizeX * 0.5f;
    vLocal.y  = 0.0f;
    vLocal.z  = g_flWallBoxSizeZ * -0.5f;
    vOrigin.z = g_flWallBoxSizeZ * 0.5f;
    vOrigin.y = g_flWallBoxTopY;

    box.bValid = 1;
    box.vMin   = vLocal;
    box.vMax   = vOrigin;

    pOther->m_transformC.TransformDirection(
        *CollisionCheck::ComputeNearestNormal(&vDelta, &box, &vPoint),
        &vLocal);

    Vec3 vNormal = vLocal;

    flSpeed = g_pGameEngine->m_clock.flInvDelta * pSelfBody->flSpeed;

    if (flSpeed > 20.0f)
    {
        float flElasticity;
        float flFriction;

        flElasticity = pOtherBody->flElasticity;
        flFriction   = pOtherBody->flFriction;

        CollisionObj plane(&vNormal, &vOtherPos, flFriction, flElasticity);

        pNormalField = &pSelfBody->surfNormal;

        CollisionShape* const pSavedSurface = pSelfBody->pShell;

        vLocal = *pNormalField;

        pSelfBody->pShell = &plane;

        *pNormalField = vNormal;

        pSelfBody->UpdateVelocityDirection();

        pSelfBody->pShell = pSavedSurface;

        *pNormalField = vLocal;

        if (pSelf->GetSlot() < PLAYER_OBJECT_COUNT)
        {
            GameBehaviorObject* pBindNode;
            float               flRatio;
            float               flFacing;
            float               flLimit;

            flLimit = pSelfBody->flForwardSpeedDt;

            flRatio = pSelfBody->flSpeed / flLimit * 4.0f;

            CRelaxYaw* const pRelax = (CRelaxYaw*)AllocAgendaItem(Agenda_CRelaxYaw);

            pBindNode = pSelf->GetPlanState()->m_pCurrent;
            pRelax->Bind(pSelf, pBindNode);
            pRelax->SetAngle(g_flHitSpin);
            pRelax->Activate();

            flFacing = g_flFacingStepBase;

            vLocal = pSelf->m_pBody->vel;

            Vec3::ScaleTo(&vLocal, flFactor, &vDelta);

            const Vec3* pCentre = pSelf->GetWorldPos(&vScratch);
            vOrigin.x           = vNormal.x - pCentre->x;

            pCentre   = pSelf->GetWorldPos(&vScratch);
            vOrigin.y = vNormal.y - pCentre->y;

            pCentre   = pSelf->GetWorldPos(&vScratch);
            vOrigin.z = vNormal.z - pCentre->z;

            Math::Vec3Normalize(&vOrigin);

            Vec3* const pDir = &pSelfBody->velDir;

            vDelta.x = vOrigin.z * pDir->y - vOrigin.y * pDir->z;
            vDelta.y = vOrigin.x * pDir->z - vOrigin.z * pDir->x;
            vDelta.z = vOrigin.y * pDir->x - vOrigin.x * pDir->y;

            if (vDelta.z <= 0.0)
            {
                if (flFacing > 0.0f)
                {
                    flFacing = 7.0f - flFacing;
                }
                else
                {
                    flFacing = 0.0f;
                }
            }

            if (flRatio > 4.0f)
            {
                g_nCheckImpulseStep = 4;
            }
            else
            {
                g_nCheckImpulseStep = (short)flRatio;
            }

            g_nCheckImpulseFacing = (short)flFacing;

            pSelf->PostSceneEvent(
                MSG_SET_ANIMATION,
                g_awReactionByDamageAndSector[g_nCheckImpulseStep * 7 + g_nCheckImpulseFacing]);
            pSelf->PostSceneEvent(MSG_YER_HIT, 0);

            const short nViewed = g_pGameEngine->m_nViewedSlot;

            if (pSelf->GetSlot() == nViewed)
            {
                EngineObject* const pAnnounce = g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN];

                pAnnounce->PostSceneEvent(MSG_HIT, pSelf->GetSlot());

                g_nCheckImpulseSelfSlot  = pSelf->GetSlot();
                g_nCheckImpulseOtherSlot = pOther->GetSlot();
            }
        }
    }
    else
    {
        Vec3* pPos;

        pNormalField = &pSelfBody->surfNormal;
        pPos         = &pSelfBody->pos;

        vDelta.y = vLocal.y * 0.5f;
        vDelta.z = vLocal.z * 0.5f;

        vScratch.x = vLocal.x * 0.5f + pPos->x;
        vScratch.y = vDelta.y + pPos->y;
        vScratch.z = vDelta.z + pPos->z;

        pSelfBody->pShell->RayIntersection(pNormalField, &vScratch, pPos);

        if (flSpeed > 3.0f)
        {
            float flDot;
            float flScale;

            vLocal = pSelfBody->surfNormal;

            vOrigin = pSelf->m_pBody->vel;

            flDot = Vec3::DotInline(&vLocal, &vOrigin);

            vDelta.x = vLocal.x * flDot;
            vDelta.y = vLocal.y * flDot;

            vOrigin.x -= vDelta.x;
            vOrigin.y -= vDelta.y;

            const float flZ = vOrigin.z - flDot * vLocal.z;

            flScale = 3.0f / (g_pGameEngine->m_clock.flInvDelta * pSelfBody->flSpeed);

            vOrigin.x = flScale * vOrigin.x;
            vOrigin.y *= flScale;
            vOrigin.z = flZ * flScale;

            pSelf->SetVelocity(vOrigin);
        }
    }
}

// Action 78: the velocity copy scaled by minus ten is overwritten unread, in the original too.

// FUNCTION: HYPERX 0x00405920
void __cdecl PlanAction::ReflectOffModel(Interactor* pContext, PlanNode* pNode, short*, int nSlot)
{
    BoundBubble*  pColOther;
    Vec3*         pNormalField;
    EngineObject* pSelf;
    EngineObject* pOther;
    Vec3          vSteer;
    Vec3          vDir;
    Vec3          vNormal;
    Vec3          vOtherPos;
    Vec3          vSelfPos;
    float         flSpeed;

    if (pContext->GetNamedSlot() == SLOT_EXPLICIT_PAIR)
    {
        BoundBubble* const pColSelf = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

        pSelf = pColSelf->GetOwner();

        pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pOther = pColOther->GetOwner();
    }
    else
    {
        // Dead for a short; the client tests the short at 0x004059b6 and indexes the int at 0x004059d0.
        if ((unsigned short)((unsigned int)(short)nSlot >> 16) == PLAN_ARG_SELECTOR)
        {
            nSlot = ((PlanStatFnWide*)g_apPlanStatFns)[nSlot & 0xffff](pContext->GetActor(), pNode);
        }

        pOther = pContext->GetActor();

        pSelf = g_pGameEngine->m_ppSlots[(short)nSlot];
    }

    PhysicsBody*       pSelfBody  = pSelf->GetPhysicsBody();
    PhysicsBody* const pOtherBody = pOther->GetPhysicsBody();

    const Vec3* pSelfCentre = pSelf->GetWorldPos(&vSteer);
    vSelfPos                = *pSelfCentre;

    vOtherPos = *pOther->GetWorldPos(&vSteer);

    vDir    = *Object::ComputeSteeringDirection(&vSteer, pOther, &vSelfPos);
    vNormal = vDir;

    Vec3 vEntryVel = pSelf->m_pBody->vel;

    flSpeed = (float)(sqrt(vEntryVel.x * vEntryVel.x + (vEntryVel.y * vEntryVel.y + vEntryVel.z * vEntryVel.z)) * g_pGameEngine->m_clock.flInvDelta);

    if (flSpeed > 20.0f)
    {
        Vec3  vSaved;
        float flElasticity;
        float flFriction;

        flElasticity = pOtherBody->flElasticity;
        flFriction   = pOtherBody->flFriction;

        CollisionObj plane(&vNormal, &vOtherPos, flFriction, flElasticity);

        CollisionShape* const pSavedSurface = pSelfBody->pShell;
        pNormalField                        = &pSelfBody->surfNormal;

        vSaved = *pNormalField;

        pSelfBody->pShell = &plane;

        *pNormalField = vNormal;

        pSelfBody->UpdateVelocityDirection();

        pSelfBody->pShell = pSavedSurface;

        *pNormalField = vSaved;

        Vec3        vSlideVel;
        const Vec3* pSaved;
        float       flDot;
        float       flProjX;
        float       flProjY;
        float       flProjZ;

        vSlideVel = pSelf->m_pBody->vel;

        pSaved = &vSaved;

        flDot = pSaved->x * vSlideVel.x + (vSlideVel.y * pSaved->y + vSlideVel.z * pSaved->z);

        flProjX = pSaved->x * flDot;
        flProjY = pSaved->y * flDot;
        flProjZ = flDot * pSaved->z;

        vSlideVel.x -= flProjX;
        vSlideVel.y -= flProjY;
        vSlideVel.z -= flProjZ;

        pSelf->SetVelocity(vSlideVel);

        if (pSelf->GetSlot() < PLAYER_OBJECT_COUNT)
        {
            Vec3                vDiff;
            Vec3                vTemp;
            GameBehaviorObject* pBindNode;
            float               flRatio;
            float               flFacing;
            float               flLimit;
            float               flSpin;

            flLimit = pSelfBody->flForwardSpeedDt;

            flRatio = pSelfBody->flSpeed / flLimit * 4.0f;

            CRelaxYaw* const pRelax = (CRelaxYaw*)AllocAgendaItem(Agenda_CRelaxYaw);

            pBindNode = pSelf->GetPlanState()->m_pCurrent;
            pRelax->Bind(pSelf, pBindNode);
            flSpin            = g_flHitSpin;
            pRelax->m_flAngle = flSpin;
            pRelax->Activate();

            flFacing = g_flFacingStepBase;

            {
                Vec3 vVel;

                vVel = pSelf->m_pBody->vel;

                Vec3::ScaleTo(&vVel, -10.0f, &vTemp);
            }

            {
                Vec3 vOtherScratch;

                pSelfCentre              = pSelf->GetWorldPos(&vSteer);
                const Vec3* pOtherCentre = pOther->GetWorldPos(&vOtherScratch);
                vDiff.x                  = pSelfCentre->x - pOtherCentre->x;

                pSelfCentre  = pSelf->GetWorldPos(&vSteer);
                pOtherCentre = pOther->GetWorldPos(&vOtherScratch);
                vDiff.y      = pSelfCentre->y - pOtherCentre->y;

                pSelfCentre  = pSelf->GetWorldPos(&vSteer);
                pOtherCentre = pOther->GetWorldPos(&vOtherScratch);
                vDiff.z      = pSelfCentre->z - pOtherCentre->z;
            }

            Math::Vec3Normalize(&vDiff);

            Vec3* const pDir = &pSelfBody->velDir;

            vTemp.x = vDiff.z * pDir->y - vDiff.y * pDir->z;
            vTemp.y = vDiff.x * pDir->z - vDiff.z * pDir->x;
            vTemp.z = vDiff.y * pDir->x - vDiff.x * pDir->y;

            if (vTemp.z <= 0.0)
            {
                if (flFacing > 0.0f)
                {
                    flFacing = 7.0f - flFacing;
                }
                else
                {
                    flFacing = 0.0f;
                }
            }

            if (flRatio > 4.0f)
            {
                g_nCheckImpulseStep = 4;
            }
            else
            {
                g_nCheckImpulseStep = (short)flRatio;
            }

            g_nCheckImpulseFacing = (short)flFacing;

            pSelf->PostSceneEvent(
                MSG_SET_ANIMATION,
                g_awReactionByDamageAndSector[g_nCheckImpulseStep * 7 + g_nCheckImpulseFacing]);
            pSelf->PostSceneEvent(MSG_YER_HIT, 0);

            const short nViewed = g_pGameEngine->m_nViewedSlot;

            if (pSelf->GetSlot() == nViewed)
            {
                EngineObject* const pAnnounce = g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN];

                pAnnounce->PostSceneEvent(MSG_HIT, pSelf->GetSlot());

                g_nCheckImpulseSelfSlot  = pSelf->GetSlot();
                g_nCheckImpulseOtherSlot = pOther->GetSlot();
            }
        }
    }
    else
    {
        Vec3            vPush;
        Vec3            vSum;
        Vec3*           pPos;
        CollisionShape* pShell;

        pNormalField = &pSelfBody->surfNormal;
        pPos         = &pSelfBody->pos;
        pShell       = pSelfBody->pShell;

        vPush.x = vDir.x * 0.5f;
        vPush.y = vDir.y * 0.5f;
        vPush.z = vDir.z * 0.5f;

        vSum.x = vPush.x + pPos->x;
        vSum.y = vPush.y + pPos->y;
        vSum.z = vPush.z + pPos->z;

        vPush = vSum;

        pShell->RayIntersection(pNormalField, &vPush, pPos);

        if (flSpeed > 3.0f)
        {
            Vec3  vVel;
            Vec3  vSurface;
            float flDot;
            float flScale;

            vSurface = pSelfBody->surfNormal;

            vVel = pSelf->m_pBody->vel;

            flDot = vSurface.x * vVel.x + (vVel.y * vSurface.y + vVel.z * vSurface.z);

            vPush.x = vSurface.x * flDot;
            vPush.y = vSurface.y * flDot;

            vVel.x -= vPush.x;
            vVel.y -= vPush.y;
            vVel.z -= flDot * vSurface.z;

            flScale = 3.0f / (g_pGameEngine->m_clock.flInvDelta * pSelfBody->flSpeed);

            vVel.x = flScale * vVel.x;
            vVel.y *= flScale;
            vVel.z *= flScale;

            pSelf->SetVelocity(vVel);
        }
    }
}

// Slows the collision's own object to three tenths of its velocity and picks its reaction animation.

// FUNCTION: HYPERX 0x00405fd0
void __cdecl PlanAction::ClothesLineRecoil(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* pAnnounce;
    Vec3          vTemp;
    Vec3          vDiff;

    BoundBubble* const pColSelf = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject* pSelf = pColSelf->GetOwner();

    BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject* const pOther = pColOther->GetOwner();

    PhysicsBody* pBody = pSelf->GetPhysicsBody();

    Vec3::ScaleTo(&pBody->vel, 0.3f, &vTemp);

    const float flRatio = pBody->flSpeed / pBody->GetForwardSpeedDt() * 4.0f;

    pSelf->SetVelocity(vTemp);

    float flFacing = g_flFacingStepBase;

    {
        Vec3 vSelfScratch;
        Vec3 vOtherScratch;

        const Vec3* pOtherX = pOther->GetWorldPos(&vOtherScratch);
        const Vec3* pSelfX  = pSelf->GetWorldPos(&vSelfScratch);
        vDiff.x             = pOtherX->x - pSelfX->x;

        const Vec3* pOtherY = pOther->GetWorldPos(&vOtherScratch);
        const Vec3* pSelfY  = pSelf->GetWorldPos(&vSelfScratch);
        vDiff.y             = pOtherY->y - pSelfY->y;

        const Vec3* pOtherZ = pOther->GetWorldPos(&vOtherScratch);
        const Vec3* pSelfZ  = pSelf->GetWorldPos(&vSelfScratch);
        vDiff.z             = pOtherZ->z - pSelfZ->z;
    }

    Math::Vec3Normalize(&vDiff);

    Vec3* const pDir = &pSelf->GetPhysicsBody()->velDir;

    vTemp.x = vDiff.z * pDir->y - vDiff.y * pDir->z;
    vTemp.y = vDiff.x * pDir->z - vDiff.z * pDir->x;
    vTemp.z = vDiff.y * pDir->x - vDiff.x * pDir->y;

    if (vTemp.z <= 0.0)
    {
        if (flFacing > 0.0f)
        {
            flFacing = 7.0f - flFacing;
        }
        else
        {
            flFacing = 0.0f;
        }
    }

    if (flRatio > 4.0f)
    {
        g_nCheckImpulseStep = 4;
    }
    else
    {
        g_nCheckImpulseStep = (short)flRatio;
    }

    g_nCheckImpulseFacing = (short)flFacing;

    pSelf->PostSceneEvent(MSG_REALLY_SET_ANIMATION, g_awReactionByDamageAndSector[g_nCheckImpulseStep * 7 + g_nCheckImpulseFacing]);
    pSelf->PostSceneEvent(MSG_YER_HIT, 0);

    const short nViewed = g_pGameEngine->m_nViewedSlot;

    if (pSelf->GetSlot() == nViewed)
    {
        pAnnounce = g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN];

        pAnnounce->PostSceneEvent(MSG_HIT, pSelf->GetSlot());

        g_nCheckImpulseSelfSlot  = pSelf->GetSlot();
        g_nCheckImpulseOtherSlot = pOther->GetSlot();
    }

    int nOtherSlot = pOther->GetSlot();

    if (g_pGameEngine->m_nBrutality != 0)
    {
        if (nOtherSlot >= SLOT_TURNSTILE && nOtherSlot <= SLOT_TURNSTILE_LAST)
        {
            ParticleSystem* const pSystem = g_pGameEngine->FindFreeParticleSystem();

            if (pSystem != 0)
            {
                Vec3 vPosition;
                Vec3 vVelocity;

                vPosition = *pSelf->GetPositionPtr();

                vVelocity = pSelf->m_pBody->vel;

                vPosition.z += 3.0f;

                pSystem->SetOrientation(&vPosition, &vVelocity);
                pSystem->SpawnParticles(10.0f, 50.0f);

                AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CExplosionAnimation);

                PlanState* const pPlan = pSystem->GetPlanState();

                pAgendaItem->Bind(pSystem, pPlan->m_pCurrent);
                pAgendaItem->Activate();
            }
        }
    }
}

// A team mate's shove weighs a quarter and never triggers the stopped-body reaction.

// Speed counts five times over in the weight, and a thousandth keeps it off nought.
#define SPEED_IN_WEIGHT 5.0f
#define WEIGHT_FLOOR    0.001f

// FUNCTION: HYPERX 0x00406770
void __cdecl PlanAction::ManOnManRecoil(Interactor* pContext, PlanNode*, short*, int)
{
    float flSelfWeight;
    float flOtherWeight;
    int   nSelfSector;
    int   nOtherSector;

    BoundBubble* const pColSelf = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject* pSelf = pColSelf->GetOwner();

    BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject* pOther = pColOther->GetOwner();

    if (((unsigned char)pSelf->m_nFlags & ENGINEOBJECT_HIDDEN) == 0 && ((unsigned char)pOther->m_nFlags & ENGINEOBJECT_HIDDEN) == 0)
    {
        const Vec3* pOtherVel = &pOther->GetPhysicsBody()->vel;
        const Vec3* pSelfVel  = &pSelf->GetPhysicsBody()->vel;
        const Vec3* pSelfDir  = &pSelf->GetPhysicsBody()->velDir;
        const Vec3* pOtherDir = &pOther->GetPhysicsBody()->velDir;

        float flRoot = pOtherVel->Magnitude();

        const float flOtherSpeed = flRoot;

        flRoot = pSelfVel->Magnitude();

        const float flFacingDot = Vec3::DotInline(pOtherDir, pSelfDir);

        int nSelfStopped = 1;

        if (pSelfVel->x + (pSelfVel->GetY() + pSelfVel->z) != 0.0f)
        {
            nSelfStopped = 0;
        }

        int nOtherStopped = 1;

        if (pOtherVel->x + pOtherVel->y + pOtherVel->z != 0.0f)
        {
            nOtherStopped = 0;
        }

        if (fabs(pOtherVel->x - pSelfVel->x + pOtherVel->y - pSelfVel->y + pOtherVel->z - pSelfVel->z) < g_pGameEngine->m_clock.flDelta)
        {
            pSelf->GetSlot();
            pOther->GetSlot();
            return;
        }

        flOtherWeight = ((PlayerObject*)pOther)->m_stats.GetWeightedStat(RATING_WEIGHT_CLASS) + flOtherSpeed * SPEED_IN_WEIGHT + WEIGHT_FLOOR;
        flSelfWeight  = flRoot * SPEED_IN_WEIGHT + ((PlayerObject*)pSelf)->m_stats.GetWeightedStat(RATING_WEIGHT_CLASS) + WEIGHT_FLOOR;

        const int nOtherParity = pOther->GetSlot() % 2;

        const int nSelfParity = pSelf->GetSlot() % 2;

        if (nSelfParity == nOtherParity)
        {
            flSelfWeight *= 0.25f;
            nSelfStopped  = 0;
            nOtherStopped = 0;

            flOtherWeight *= 0.25f;
        }

        nSelfSector  = EngineObject::GetApproachAngleSector(pSelf,
                                                            pOther);
        nOtherSector = EngineObject::GetApproachAngleSector(pOther,
                                                            pSelf);

        Object::HandleObjectCollisionBounce(pSelf, pOther, pSelfVel, pOtherVel, flFacingDot, flSelfWeight, flOtherWeight, nSelfStopped, nOtherStopped, nSelfSector, nOtherSector, flRoot, flOtherSpeed);

        Object::HandleObjectCollisionBounce(pOther, pSelf, pOtherVel, pSelfVel, flFacingDot, flOtherWeight, flSelfWeight, nOtherStopped, nSelfStopped, nOtherSector, nSelfSector, flOtherSpeed, flRoot);

        short nViewed = g_pGameEngine->m_nViewedSlot;

        if (pSelf->GetSlot() != nViewed)
        {
            nViewed = g_pGameEngine->m_nViewedSlot;

            if (pOther->GetSlot() != nViewed)
            {
                return;
            }
        }

        EngineObject* const pSoundMan = g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN];

        pSoundMan->PostSceneEvent(MSG_HIT, pSelf->GetSlot());
    }
}

// A nearly stopped object leaves along the other's reversed velocity, and always upwards.

// FUNCTION: HYPERX 0x00406ba0
void __cdecl PlanAction::BounceOffDromer(Interactor* pContext, PlanNode* pParams, short*, int)
{
    Vec3 vDir;

    BoundBubble* const pColOther = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject* pOther = pColOther->GetOwner();

    BoundBubble* const pColSelf = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject* const pSelf = pColSelf->GetOwner();

    if (pSelf != 0 && pOther != 0)
    {
        Vec3* const pSelfX  = pSelf->GetPositionPtr();
        Vec3* const pOtherX = pOther->GetPositionPtr();
        vDir.x              = pSelfX->x - pOtherX->x;

        Vec3* const pSelfY  = pSelf->GetPositionPtr();
        Vec3* const pOtherY = pOther->GetPositionPtr();
        vDir.y              = pSelfY->y - pOtherY->y;

        Vec3* const pSelfZ  = pSelf->GetPositionPtr();
        Vec3* const pOtherZ = pOther->GetPositionPtr();
        vDir.z              = pSelfZ->z - pOtherZ->z;

        Math::Vec3Normalize(&vDir);

        float flScale = 1.0f;

        if (pParams->m_nArgCount == 2)
        {
            flScale = pParams->m_pArgs[1].n * 0.01f;
        }

        Vec3 vVel = pSelf->GetPhysicsBody()->vel;

        if (pSelf->GetPhysicsBody()->flSpeed < 0.1)
        {
            vVel = pOther->GetPhysicsBody()->vel;
            vVel.x *= -1.0f;
            vVel.y *= -1.0f;
            vVel.z *= -1.0f;
        }

        const float flDot = Vec3::DotInline(&vVel, &vDir);

        const float flNegY = -(flDot * vDir.y);
        const float flNegZ = -(flDot * vDir.z);
        const float flNegX = -(flDot * vDir.x);

        vVel.x = flNegX * flScale + (vVel.x + flNegX) * 0.3f;
        vVel.y = flNegY * flScale + (vVel.GetY() + flNegY) * 0.3f;
        vVel.z = (flNegZ * flScale + (vVel.GetZ() + flNegZ) * 0.3f) * 1.2f;

        if (vVel.z < 0.0f)
        {
            vVel.z = -vVel.z;
        }

        pSelf->SetVelocity(vVel);
    }
}
