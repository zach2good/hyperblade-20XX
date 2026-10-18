#include "common/compat.h"
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

constexpr auto kArgMessage = 1;

constexpr auto kArgCountWithMessage = 2;

// The second object's state block is fetched and never used, in the original too.

// FUNCTION: HYPERX 0x0046a9e0
void PlanAction::GlideOnRamp(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3 vPoint;

    BoundBubble* pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject* const pObject = pCollision->GetOwner();

    pCollision = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    EngineObject* const pOther = pCollision->GetOwner();

    PhysicsBody* const pState = pObject->GetPhysicsBody();

    pOther->GetPhysicsBody();

    Vec3* const pPoint = pObject->GetWorldPos(&vPoint);

    CollisionPlane* const pPart = ((RampObject*)pOther)->GetCollisionObjForPoint(pPoint);

    pState->pShell = pPart;

    pState->surfNormal = *pPart->GetNormal();
}

constexpr auto kInfoBarHoldMs = 0x5dc;

constexpr auto kMessageKindEven = 1;
constexpr auto kMessageKindOdd  = 2;

// FUNCTION: HYPERX 0x0046aa90
void PlanAction::Trace(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    if (pArgs->m_nArgCount > 1)
    {
        pContext->GetActor();

        pContext->GetActor()->GetPlanState();
        pContext->GetActor()->GetPlanState();
    }
}

// FUNCTION: HYPERX 0x0046aad0
void PlanAction::SetInfoBarText(Interactor*, PlanNode* pNode, short*, int)
{
    if (pNode->m_nArgCount == kArgCountWithMessage)
    {
        const int nIndex = pNode->m_pArgs[kArgMessage].n;
        const int nFirst = g_pMessageQueue->GetAt(nIndex);

        g_pMessageQueue->PushFromTable(nIndex, nFirst, kInfoBarHoldMs);
    }
}

// The kind alternates with the slot's low bit so the two sides of a pairing get different rows.

// FUNCTION: HYPERX 0x0046ab10
void PlanAction::SetInfoTextDromer(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    const char* pszName;
    int         nKind;

    const PlanWord* pBlock = pArgs->m_pArgs;
    PlanWord        slot   = pBlock[1];
    const int       nIndex = pBlock[2].n;

    if (slot.IsPronoun())
    {
        PlanStatFn pfnResolver = g_apPlanStatFns[slot.wPronoun];

        slot.n = pfnResolver(pContext->GetActor(), pArgs);
    }

    if (slot.n == kSlotNone)
    {
        return;
    }

    if ((slot.n & 1) != 0)
    {
        pszName = g_pGameEngine->m_ppSlots[static_cast<short>(slot.n)]->m_pszName;
        nKind   = kMessageKindOdd;
    }
    else
    {
        pszName = g_pGameEngine->m_ppSlots[static_cast<short>(slot.n)]->m_pszName;
        nKind   = kMessageKindEven;
    }

    g_pMessageQueue->PushNamedFromTable(pszName, nIndex, nKind, kInfoBarHoldMs);
}

// A slot of minus one means the actor has no team and nothing is pushed.
// FUNCTION: HYPERX 0x0046aba0
void PlanAction::SetInfoTextTeam(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    const char* pTeam;
    int         nKind;
    int         nSlot;

    const int nIndex = pArgs->m_pArgs[1].n;

    if (pArgs->m_nArgCount == 2)
    {
        nSlot = pContext->GetActor()->GetSlot();

        if (nSlot != kSlotNone)
        {
            if ((nSlot & 1) != 0)
            {
                nKind = kMessageKindOdd;
                pTeam = g_pGameEngine->m_pTeamB->GetName();
            }
            else
            {
                pTeam = g_pGameEngine->m_pTeamA->GetName();
                nKind = kMessageKindEven;
            }

            g_pMessageQueue->PushNamedFromTable(pTeam, nIndex, nKind, kInfoBarHoldMs);
        }
    }
}

// Set to one so the first save through each row wins.
// GLOBAL: HYPERX 0x004c453c
extern short g_awColorLatched[2];
short        g_awColorLatched[2] = { 1, 1 };

extern float g_aOpposSpeedCache[4];
float        g_aOpposSpeedCache[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

extern short g_nOpposSpeedCacheStale;
short        g_nOpposSpeedCacheStale = 1;

// Every entry ships at minus one, which the action tests before it writes anything.
extern float g_aSpeedCache[4];
float        g_aSpeedCache[4] = { -1.0f, -1.0f, -1.0f, -1.0f };

extern short g_nSpeedCacheStale;
short        g_nSpeedCacheStale = 1;
