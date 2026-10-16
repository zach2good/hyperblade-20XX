#include "common/compat.h"
#include "engine/messageids.h"
#include "physics/physicsbody.h"
#include "devicector.h"
#include "math/vector.h"
#include "physics/upaxis.h"
#include "math/transformobject.h"
#include "planvm/damagegate.h"
#include "graphics/meshboundingbox.h"
#include "gameengine.h"
#include "globals.h"
#include "graphics/meshbone.h"
#include "graphics/graphicsglobals.h"
#include "input/inputglobals.h"
#include "math/mathglobals.h"
#include <math.h>
#include "engine/engineobject.h"
#include "physics/collisioncheck.h"
#include "engine/object.h"
#include "engine/objectclasses.h"

// The post width, the span across both posts, and the height: three, nine and ten.
// GLOBAL: HYPERX 0x004bb394
extern float g_flDromePostWidth;

// GLOBAL: HYPERX 0x004bb39c
extern float g_flDromeHeight;

// GLOBAL: HYPERX 0x004bb398
extern float g_flDromeSpanWidth;

// The receiving object always leaves upwards; the translation is not copied.
// FUNCTION: HYPERX 0x00402dc0
void __cdecl Object::CopyPhysicsStateTo(EngineObject* pSrc, EngineObject* pDest, char bBoost)
{
    Vec3 vVel;

    pDest->m_pBody->pos = pSrc->GetPhysicsBody()->pos;

    pDest->GetLocalTransformPtr()->ResetToIdentity();
    pDest->GetLocalTransformPtr()->SetTranslation(0, 0, 0);
    // Slot 0x64, the render slot pose, between the transform reset and the transform copy.
    pDest->ApplyRenderSlotPose();

    TransformObject* const pDestMotion = pDest->GetLocalTransformPtr();
    TransformObject* const pSrcMotion  = pSrc->GetLocalTransformPtr();

    pDestMotion->CopyRotation(pSrcMotion);

    if (bBoost != 0)
    {
        PhysicsBody* pSrcBody = pSrc->GetPhysicsBody();

        Vec3::ScaleTo(&pSrcBody->vel, 1.2f, &vVel);
    }
    else
    {
        vVel = pSrc->GetPhysicsBody()->vel;
    }

    if (vVel.z < 0.0)
    {
        vVel.z = -vVel.z;
    }

    pDest->SetVelocity(vVel);

    pDest->GetPhysicsBody()->flFacing = 0.0f;

    if (bBoost != 0)
    {
        pDest->PostSceneEvent(MSG_JUMP, 0);
        return;
    }

    pDest->PostSceneEvent(MSG_LOST_IT, 0);
}

// Puts one object six units to one side of another.
// FUNCTION: HYPERX 0x00402f20
void __cdecl Object::SpawnAtObjectRotated(EngineObject* pSrc, EngineObject* pDest, char bMirrored)
{
    Vec3 vOffset;
    Vec3 vScaled;

    Vec3* const pRow = &pSrc->GetPhysicsBody()->velDir;

    if (bMirrored != 0)
    {
        vOffset.x = g_vUpAxis.z * pRow->y - g_vUpAxis.y * pRow->z;
        vOffset.y = g_vUpAxis.x * pRow->z - g_vUpAxis.z * pRow->x;
        vOffset.z = g_vUpAxis.y * pRow->x - g_vUpAxis.x * pRow->y;
    }
    else
    {
        vOffset.x = g_vUpAxis.y * pRow->z - g_vUpAxis.z * pRow->y;
        vOffset.y = g_vUpAxis.z * pRow->x - g_vUpAxis.x * pRow->z;
        vOffset.z = g_vUpAxis.x * pRow->y - g_vUpAxis.y * pRow->x;
    }

    PhysicsBody* const pBody    = pSrc->GetPhysicsBody();
    Vec3               vBase    = pBody->pos;
    Vec3*              pDestPos = &pDest->m_pBody->pos;

    const Vec3* pOffset = &vOffset;
    vScaled.x           = vOffset.x * 6.0f;
    vScaled.y           = pOffset->y * 6.0f;
    vScaled.z           = pOffset->z * 6.0f;

    vOffset.x = vBase.x + vScaled.x;
    vOffset.y = vBase.y + vScaled.y;
    vOffset.z = vBase.z + vScaled.z;

    *pDestPos = vOffset;

    pDest->GetLocalTransformPtr()->ResetToIdentity();
    pDest->GetLocalTransformPtr()->SetTranslation(0, 0, 0);
    // Slot 0x64, the render slot pose, between the transform reset and the transform copy.
    pDest->ApplyRenderSlotPose();

    TransformObject* const pDestMotion = pDest->GetLocalTransformPtr();
    TransformObject* const pSrcMotion  = pSrc->GetLocalTransformPtr();

    pDestMotion->CopyRotation(pSrcMotion);

    pDest->SetVelocity(pSrc->GetPhysicsBody()->vel);

    pDest->GetPhysicsBody()->flFacing = 0.0f;

    pDest->PostSceneEvent(MSG_DEKE, 0);
}

// Eight and two, the box's extents in x and z. Their markers are in src/ui/hudglobals.h.
extern const float g_flSteerBoxSizeX;
extern const float g_flSteerBoxSizeZ;

// Five, the box's top; its bottom is nought.
// GLOBAL: HYPERX 0x004b153c
extern const float g_flSteerBoxTopY;

// Picks one of the gate's two posts or the bar between them by where the point is.
// FUNCTION: HYPERX 0x00404670
Vec3* __cdecl Object::ComputePoleGateNormal(Vec3* pDirOut, Vec3* pQueryPos)
{
    MeshBoundingBox zone;
    MeshBoundingBox box;
    Vec3            vNormal;
    float           flOffset;
    float           flPost;

    const float flZone = g_flDromePostWidth * 10.0f;

    const float flHalf   = flZone * 0.5f;
    const float flMirror = flZone * -0.5f;
    const float flCentre = flZone * 0.5f;

    zone.vMin.x = flCentre - flHalf;
    zone.vMin.z = -(flZone * 0.5f);
    zone.vMax.x = flHalf + flCentre;
    zone.vMax.y = g_flDromeHeight;
    zone.vMax.z = flZone * 0.5f;

    box.vMin.x = flMirror - flHalf;
    box.vMin.z = -(flZone * 0.5f);
    box.vMax.x = flHalf + flMirror;
    box.vMax.y = g_flDromeHeight;
    box.vMax.z = flZone * 0.5f;

    if (zone.vMin.x <= pQueryPos->x && pQueryPos->x <= zone.vMax.x && 0.0f <= pQueryPos->y && pQueryPos->y <= zone.vMax.y && zone.vMin.z <= pQueryPos->z && pQueryPos->z <= zone.vMax.z)
    {
        flOffset = (g_flDromeSpanWidth - g_flDromePostWidth) * 0.5f;
        flPost   = g_flDromePostWidth * 0.5f;

        box.bValid = 1;
        box.vMin.y = 0.0f;
        box.vMin.x = flOffset - flPost;
        box.vMin.z = -flPost;
        box.vMax.x = flPost + flOffset;
        box.vMax.y = g_flDromeHeight;
        box.vMax.z = flPost;
    }
    else if (box.vMin.x <= pQueryPos->x && pQueryPos->x <= box.vMax.x && 0.0f <= pQueryPos->y && pQueryPos->y <= box.vMax.y && box.vMin.z <= pQueryPos->z && pQueryPos->z <= box.vMax.z)
    {
        flOffset = (g_flDromeSpanWidth - g_flDromePostWidth) * -0.5f;
        flPost   = g_flDromePostWidth * 0.5f;

        box.bValid = 1;
        box.vMin.y = 0.0f;
        box.vMin.x = flOffset - flPost;
        box.vMin.z = -flPost;
        box.vMax.x = flPost + flOffset;
        box.vMax.y = g_flDromeHeight;
        box.vMax.z = flPost;
    }
    else
    {
        const float flHalfSpan = g_flDromeSpanWidth * 0.5f;

        box.bValid = 1;
        box.vMin.y = 0.0f;
        box.vMin.x = -flHalfSpan;

        flPost = g_flDromePostWidth * 0.5f;

        box.vMin.z = -flPost;
        box.vMax.x = flHalfSpan;
        box.vMax.y = g_flDromeHeight;
        box.vMax.z = flPost;
    }

    Vec3 vResult = *CollisionCheck::ComputeNearestNormal(&vNormal, &box, pQueryPos);

    *pDirOut = vResult;

    return pDirOut;
}

namespace Object
{

// The query point relative to the transform's translation, in its rotated frame.
static inline Vec3 ToObjectFrame(EngineObject* pObject, const Vec3* pQueryPos)
{
    Vec3 vCentre;
    Vec3 vLocal;
    Vec3 vScratch;

    vCentre.x = pObject->m_transformC.m_a[0][3];
    vCentre.y = pObject->m_transformC.m_a[1][3];
    vCentre.z = pObject->m_transformC.m_a[2][3];

    Vec3* const pDelta = pQueryPos->Subtract(&vScratch, &vCentre);

    vLocal = *pDelta;

    TransformObject* const pTransform = &pObject->m_transformC;

    pTransform->TransformDirectionByRows(*pDelta, &vLocal);

    return vLocal;
}

} // namespace Object

// In the original, a slot no range claims returns whatever the frame held.
// FUNCTION: HYPERX 0x00404910
Vec3* __cdecl Object::ComputeSteeringDirection(Vec3* pDirOut, EngineObject* pObject, const Vec3* pQueryPos)
{
    Vec3 vResult = {};
    Vec3 vPoint;

    vPoint = Object::ToObjectFrame(pObject, pQueryPos);

    TransformObject* const pTransform = &pObject->m_transformC;

    const int nSlot = pObject->GetSlot();

    if (nSlot >= SLOT_MULTIPLIER && nSlot <= SLOT_MULTIPLIER_LAST)
    {
        Vec3 vPos;

        vResult   = pQueryPos->Minus(pObject->GetWorldPos(&vPos));
        vResult.z = 0.0f;

        Math::Vec3Normalize(&vResult);
    }
    else if (nSlot >= SLOT_HURDLE && nSlot <= SLOT_HURDLE_LAST)
    {
        Vec3 vMin;
        Vec3 vMax;

        vMin.x = g_flSteerBoxSizeX * -0.5f;
        vMax.x = g_flSteerBoxSizeX * 0.5f;
        vMin.z = g_flSteerBoxSizeZ * -0.5f;
        vMax.z = g_flSteerBoxSizeZ * 0.5f;
        vMin.y = 0.0f;
        vMax.y = g_flSteerBoxTopY;

        MeshBoundingBox box;

        box.bValid = 1;
        box.vMin   = vMin;
        box.vMax   = vMax;

        Vec3 vNormal;
        Vec3 vDir;

        pTransform->TransformDirection(*CollisionCheck::ComputeNearestNormal(&vNormal, &box, &vPoint),
                                       &vDir);

        vResult = vDir;
    }
    else if ((nSlot >= SLOT_LIMBO_POLE && nSlot <= SLOT_LIMBO_POLE_LAST) || (nSlot >= SLOT_LASER_HURDLE && nSlot <= SLOT_LASER_HURDLE_LAST))
    {
        Vec3 vNormal;
        Vec3 vDir;

        pTransform->TransformDirection(*Object::ComputePoleGateNormal(&vNormal, &vPoint),
                                       &vDir);

        vResult = vDir;
    }
    else if (nSlot >= SLOT_TURNSTILE && nSlot <= SLOT_TURNSTILE_LAST)
    {
        Vec3 vPos;

        vResult   = pQueryPos->Minus(pObject->GetWorldPos(&vPos));
        vResult.z = 0.0f;

        Math::Vec3Normalize(&vResult);
    }
    else if (nSlot >= SLOT_GREEN_RANDOMIZER && nSlot <= SLOT_RED_RANDOMIZER_LAST)
    {
        Vec3 vPos;

        vResult   = pQueryPos->Minus(pObject->GetWorldPos(&vPos));
        vResult.z = 0.0f;

        Math::Vec3Normalize(&vResult);
    }
    else if (nSlot >= SLOT_ARMORY && nSlot <= SLOT_ARMORY_LAST)
    {
        Vec3 vPos;

        vResult   = pQueryPos->Minus(pObject->GetWorldPos(&vPos));
        vResult.z = 0.0f;

        Math::Vec3Normalize(&vResult);
    }
    else if (nSlot == SLOT_MINE)
    {
        Vec3 vPos;

        vResult   = pQueryPos->Minus(pObject->GetWorldPos(&vPos));
        vResult.z = 0.0f;

        Math::Vec3Normalize(&vResult);
    }
    else if (nSlot == SLOT_SWARF)
    {
        Vec3 vPos;

        vResult   = pQueryPos->Minus(pObject->GetWorldPos(&vPos));
        vResult.z = 0.0f;

        Math::Vec3Normalize(&vResult);
    }

    *pDirOut = vResult;

    return pDirOut;
}

// Seven approach sectors, and seven meaning none.
#define SECTOR_NONE          7
#define SECTOR_COUNT         7
#define REACTION_SET_STOPPED 4

// HITANIMS.LST's five rows of seven sectors: 0 to 3 by damage, and 4 for the stopped case.
// GLOBAL: HYPERX 0x004cf9fc
extern short g_awReactionByDamageAndSector[35];

// Contact with no energy does not nudge the local player; the reaction is sent as
// MSG_SET_ANIMATION and then MSG_YER_HIT.
// FUNCTION: HYPERX 0x00406350
void __cdecl Object::HandleObjectCollisionBounce(EngineObject* pSelf,
                                                 EngineObject* pOther,
                                                 const Vec3*,
                                                 const Vec3*,
                                                 float flFacingDot,
                                                 float flSelfWeight,
                                                 float flOtherWeight,
                                                 int   nSelfStopped,
                                                 int   nOtherStopped,
                                                 int   nSelfSector,
                                                 int   nOtherSector,
                                                 float flSpeed,
                                                 float)
{
    Vec3  vVelocity;
    Vec3  vNormal;
    float flDelta;

    vNormal.x = pSelf->GetPositionPtr()->x - pOther->GetPositionPtr()->x;
    vNormal.y = pSelf->GetPositionPtr()->y - pOther->GetPositionPtr()->y;
    vNormal.z = pSelf->GetPositionPtr()->z - pOther->GetPositionPtr()->z;

    Math::Vec3Normalize(&vNormal);

    PhysicsBody* pBlock    = pSelf->GetPhysicsBody();
    Vec3* const  pVelocity = &pBlock->velDir;

    const float flDot = -(vNormal.x * pVelocity->x + vNormal.y * pVelocity->y + vNormal.z * pVelocity->z);

    vVelocity.x = (flDot * vNormal.x + pVelocity->x) * flSpeed;
    vVelocity.y = (flDot * vNormal.y + pVelocity->y) * flSpeed;
    vVelocity.z = (flDot * vNormal.z + pVelocity->z) * flSpeed;

    if (pSelf->GetSlot() != g_pGameEngine->GetViewedSlot() || flSpeed >= 0.05f || (float)sqrt(Vec3::DotInline(&vVelocity, &vVelocity)) >= 0.15f)
    {
        pSelf->SetVelocity(vVelocity);
    }

    if (nSelfSector == SECTOR_NONE)
    {
        nSelfSector = 0;
    }

    if (nOtherSector == SECTOR_NONE)
    {
        nOtherSector = 0;
    }

    if (flSelfWeight > flOtherWeight)
    {
        flSelfWeight = (1.0f - flFacingDot) * (flSelfWeight / (pSelf->GetPhysicsBody()->flForwardSpeedDt * 7.0f)) * 2.5f;

        pBlock = pSelf->GetPhysicsBody();

        flSpeed = flSelfWeight * flSpeed / pBlock->flForwardSpeedDt * 0.7f;

        if (flSelfWeight > 0.7f)
        {
            pSelf->PostSceneEvent(MSG_YER_HIT, 0);
            pOther->PostSceneEvent(MSG_YER_HIT, 0);
        }
    }
    else
    {
        flSpeed = (1.0f - flFacingDot) * (flOtherWeight / (pOther->GetPhysicsBody()->flForwardSpeedDt * 7.0f)) * 2.5f;

        // The original asks a second time and drops the answer.
        pOther->GetPhysicsBody();

        if (flSpeed > 0.7f)
        {
            pSelf->PostSceneEvent(MSG_YER_HIT, 0);
            pOther->PostSceneEvent(MSG_YER_HIT, 0);
        }
    }

    if (flSpeed > 4.0)
    {
        flSpeed = 4.0f;
    }

    flDelta = g_pGameEngine->m_clock.flDelta;

    if (flOtherWeight * flDelta != 0.0f)
    {
        if (nSelfStopped != 0 && nOtherStopped == 0)
        {
            pSelf->PostSceneEvent(MSG_SET_ANIMATION,
                                  g_awReactionByDamageAndSector[REACTION_SET_STOPPED * SECTOR_COUNT + nSelfSector]);
            pSelf->PostSceneEvent(MSG_YER_HIT,
                                  g_awReactionByDamageAndSector[REACTION_SET_STOPPED * SECTOR_COUNT + nOtherSector]);
        }
        else
        {
            if (flSpeed <= 0.7f)
            {
                return;
            }

            const int    nIndex = (short)flSpeed * SECTOR_COUNT + nSelfSector;
            short* const pEntry = &g_awReactionByDamageAndSector[nIndex];

            pSelf->PostSceneEvent(MSG_SET_ANIMATION, *pEntry);
            pSelf->PostSceneEvent(MSG_YER_HIT, *pEntry);
        }

        EngineObject::PostCollisionCommentary(pSelf, pOther);
    }
}

// Both scalings are cumulative, so the bone moves as well as grows.
// FUNCTION: HYPERX 0x004499d0
void Object::ScaleBones(int* pBoneIndices, int nCount, float flScaleX, float flScaleY, float flScaleZ)
{
    Vec3 vOut;

    for (int nSlot = 0; nSlot < 4; nSlot++)
    {
        EngineObject* const pObject = g_pGameEngine->m_ppSlots[(short)g_teamScaleSlots.anScaleTargetSlots[nSlot]];

        if (pObject == 0)
        {
            continue;
        }

        const int nBones = pObject->GetBoneCount();

        if (nCount <= 0)
        {
            continue;
        }

        int  nLeft  = nCount;
        int* pIndex = pBoneIndices;

        do
        {
            if (*pIndex < nBones)
            {
                MeshBone* pNode = pObject->m_pBones + (short)*pIndex;

                if (pNode != 0)
                {
                    if (pNode->IsBound() != 0)
                    {
                        TransformObject* const pWorld = &pNode->m_world;
                        Vec3                   vScale = *pWorld->GetScaleVec(&vOut);
                        vScale.x *= flScaleX;
                        vScale.y *= flScaleY;
                        pNode->m_bHasScale = 1;
                        vScale.z *= flScaleZ;
                        pWorld->m_vScale.x = vScale.x;
                        pWorld->m_vScale.y = vScale.y;
                        pWorld->m_vScale.z = vScale.z;

                        pWorld->ApplyScale();

                        TransformObject* const pLocal = &pNode->m_local;
                        Vec3                   vMoved = *pLocal->GetTranslationPtr();
                        vMoved.x *= flScaleX;
                        vMoved.y *= flScaleY;
                        vMoved.z *= flScaleZ;

                        pLocal->SetTranslation(vMoved.x, vMoved.y, vMoved.z);
                    }
                }
            }

            pIndex++;
        } while (--nLeft != 0);
    }
}

// One storage: a zero float of its own would sit in .bss, and the original's are in .data.
Vec3 g_vUpAxis = { 0.0f, 0.0f, 1.0f };

// The drome goal's post width, span across both posts, and height.
extern float g_flDromePostWidth;
float        g_flDromePostWidth = 3.0f;

extern float g_flDromeSpanWidth;
float        g_flDromeSpanWidth = 9.0f;

extern float g_flDromeHeight;
float        g_flDromeHeight = 10.0f;
