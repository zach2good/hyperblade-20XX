// Everything here except the default constructor is compiled at /Od.

#include "common/compat.h"
#include "math/transformobject.h"
#include "math/vector.h"
#include "boundbubble.h"
#include "graphics/meshbone.h"
#include "physicsbody.h"
#include "globals.h"
#include "app/gameapp.h"
#include "engine/objectphase.h"

constexpr auto kShapeWorldSpace = (-2);

// GLOBAL: HYPERX 0x004cb760
extern Vec3 g_vBubbleOwnCentre;

// GLOBAL: HYPERX 0x004cb750
extern Vec3 g_vBubbleOtherCentre;

// GLOBAL: HYPERX 0x004cb740
extern Vec3 g_vBubbleSeparation;

// FUNCTION: HYPERX 0x0042a8f0
BoundBubble::~BoundBubble()
{
    delete m_pShapeNode;

    if (m_pChildren != nullptr)
    {
        delete[] m_pChildren;

        m_pChildren = nullptr;
    }
}

// FUNCTION: HYPERX 0x0042a9a0
void BoundBubble::Init(EngineObject* pOwner, BubbleCmd* pShape)
{
    short i;

    if (pShape->m_pPos->m_nComponent == -1)
    {
        m_pTarget = &pOwner->m_transformC;
    }
    else if (pShape->m_pPos->m_nComponent >= 0)
    {
        m_pTarget = &pOwner->m_pBones[pShape->m_pPos->m_nComponent].m_world;

        pOwner->AttachBubble(pShape->m_pPos->m_nComponent, this);
    }

    m_pShapeNode   = pShape->m_pPos;
    pShape->m_pPos = nullptr;

    BubbleCmd* pChildRec = nullptr;

    m_flRadius        = m_pShapeNode->m_flRadius;
    m_nChildCount     = pShape->m_nChildCount;
    m_pOwner          = pOwner;
    m_nSlot           = pShape->m_nSlot;
    m_nShape          = pShape->m_wShape;
    m_pAttach         = pShape->m_pAttach;
    pShape->m_pAttach = nullptr;

    if (pShape->m_wShape == BUBBLE_SHAPE_ATTACHED)
    {
        FindAttachComponent(m_pAttach);
    }

    g_pGameEngine->SetColObject(m_nSlot, this);

    if (pShape->m_nChildCount > 0)
    {
        m_pChildren = new BoundBubble[pShape->m_nChildCount];

        for (i = 0; i < pShape->m_nChildCount; i++)
        {
            pChildRec = pShape->m_pChildren + i;

            switch (pChildRec->m_wShape)
            {
                case BUBBLE_SHAPE_SPHERE:
                case BUBBLE_SHAPE_ATTACHED:
                case BUBBLE_SHAPE_CAGED:
                    m_pChildren[i].Init(pOwner, pChildRec);
                    break;

                default:
                    // STRING: HYPERX 0x004bcbac
                    g_gameApp.Fatal("CBoundBubble - BAD TYPE\n");
                    break;
            }
        }
    }
    else
    {
        m_pChildren = nullptr;
    }
}

// Not finding the bone is fatal, and the message names InitAttachPaks.
// FUNCTION: HYPERX 0x0042ac80
void BoundBubble::FindAttachComponent(AttachPak* pRequest)
{
    for (short i = 0; i < m_pOwner->GetBoneCount(); i++)
    {
        const auto nLinkId = m_pOwner->m_pBones[i].m_pDesc->m_nLinkId;

        if (pRequest->nLinkId == nLinkId)
        {
            pRequest->pTransform = &m_pOwner->m_pBones[i].m_world;
            return;
        }
    }

    // STRING: HYPERX 0x004bcbc8
    g_gameApp.Fatal("InitAttachPaks: can't find component!");
}

// FUNCTION: HYPERX 0x0042ad20
void BoundBubble::GetWorldPosition(Vec3* pOut)
{
    const auto  local      = m_pShapeNode->m_vCentre;
    auto* const pTransform = m_pTarget;

    pOut->x = local.x * pTransform->m_a[0][0] + local.y * pTransform->m_a[0][1] + local.z * pTransform->m_a[0][2] + pTransform->m_a[0][3];

    pOut->y = local.x * pTransform->m_a[1][0] + local.y * pTransform->m_a[1][1] + local.z * pTransform->m_a[1][2] + pTransform->m_a[1][3];

    pOut->z = local.x * pTransform->m_a[2][0] + local.y * pTransform->m_a[2][1] + local.z * pTransform->m_a[2][2] + pTransform->m_a[2][3];
}

// Only a sphere is handled; the fatal message misspells the function it names.
// FUNCTION: HYPERX 0x0042ade0
unsigned char BoundBubble::DoesIntersect(BoundBubble* pOther)
{
    if (pOther->m_nShape == BUBBLE_SHAPE_SPHERE)
    {
        return SphereSphereOverlap(pOther);
    }

    // STRING: HYPERX 0x004bcbf0
    g_gameApp.Fatal("NO BUBBLE TYPE IN DoesInterset!");

    return 0;
}

// FUNCTION: HYPERX 0x0042ae20
unsigned char BoundBubble::SphereSphereOverlap(BoundBubble* pOther)
{
    float flRadiusSum;
    float flLenSq;

    if (m_pShapeNode->m_nComponent == kShapeWorldSpace)
    {
        g_vBubbleOwnCentre = m_pShapeNode->m_vCentre;
    }
    else
    {
        const auto                   vLocal  = m_pShapeNode->m_vCentre;
        const TransformObject* const pOrient = m_pTarget;
        g_vBubbleOwnCentre.x                 = vLocal.x * pOrient->m_a[0][0] + vLocal.y * pOrient->m_a[0][1] + vLocal.z * pOrient->m_a[0][2] + pOrient->m_a[0][3];
        g_vBubbleOwnCentre.y                 = vLocal.x * pOrient->m_a[1][0] + vLocal.y * pOrient->m_a[1][1] + vLocal.z * pOrient->m_a[1][2] + pOrient->m_a[1][3];
        g_vBubbleOwnCentre.z                 = vLocal.x * pOrient->m_a[2][0] + vLocal.y * pOrient->m_a[2][1] + vLocal.z * pOrient->m_a[2][2] + pOrient->m_a[2][3];
    }

    auto* const pShapeRec = pOther->m_pShapeNode;

    if (pShapeRec->m_nComponent == kShapeWorldSpace)
    {
        g_vBubbleOtherCentre = pShapeRec->m_vCentre;
    }
    else
    {
        pOther->GetWorldPosition(&g_vBubbleOtherCentre);
    }

    g_vBubbleSeparation.x = g_vBubbleOwnCentre.x - g_vBubbleOtherCentre.x;
    g_vBubbleSeparation.y = g_vBubbleOwnCentre.y - g_vBubbleOtherCentre.y;
    g_vBubbleSeparation.z = g_vBubbleOwnCentre.z - g_vBubbleOtherCentre.z;

    flLenSq     = g_vBubbleSeparation.x * g_vBubbleSeparation.x + g_vBubbleSeparation.y * g_vBubbleSeparation.y + g_vBubbleSeparation.z * g_vBubbleSeparation.z;
    flRadiusSum = m_flRadius + pOther->m_flRadius;

    if (flRadiusSum * flRadiusSum > flLenSq)
    {
        if (GetOwner()->GetPhase() == ObjectPhase::kLocal || pOther->GetOwner()->GetPhase() == ObjectPhase::kLocal)
        {
            return 1;
        }
    }

    return 0;
}

// The array element's default constructor, which new BoundBubble[n] names.
// FUNCTION: HYPERX 0x0042b000
BoundBubble::BoundBubble()
: m_pShapeNode(nullptr)
, m_flRadius(g_flZeroFaceNormal)
{
}
