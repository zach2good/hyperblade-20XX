// The runtime selector bank at 0x004d1798, which answers from the engine's focus slot.

#include "engine/gameengine.h"
#include "common/compat.h"
#include "engine/cameraobject.h"
#include "engine/objectclasses.h"
#include "globals.h"
#include "planfocusarg.h"
#include "planvm/plannode.h"
#include "engine/collisionobjectids.h"
#include "engine/playerobject.h"
#include "engine/rokobject.h"
#include "engine/cbeattached.h"
#include "physics/physicsbody.h"

// FUNCTION: HYPERX 0x0047cb30
short PlanFocusArg::GetViewOwner(EngineObject*, PlanNode*)
{
    return g_pGameEngine->m_nViewedSlot;
}

// Three times the focus slot, mirroring PlanArg::GetMyCollisionBub.

// FUNCTION: HYPERX 0x0047cb40
short PlanFocusArg::GetViewOwnerCOB(EngineObject*, PlanNode*)
{
    return (short)(g_pGameEngine->m_nViewedSlot * 3);
}

// The view owner's awareness bubble, mirroring PlanArg::GetMyAwarenessBub.

// FUNCTION: HYPERX 0x0047cb50
short PlanFocusArg::GetVwOwnrAwarnsBub(EngineObject*, PlanNode*)
{
    return (short)(g_pGameEngine->m_nViewedSlot + COB_DR0_AWARENESS);
}

// Three times the parity opposite the focus slot's, mirroring PlanArg::GetMyLeadOppnCollBub.

// FUNCTION: HYPERX 0x0047cb60
short PlanFocusArg::GetViewOnrsOppn1COB(EngineObject*, PlanNode*)
{
    const char cFocus = (char)g_pGameEngine->m_nViewedSlot;

    return (short)(((char)~cFocus & 1) * 3);
}

// The view owner's other opponent's collision bubble, mirroring PlanArg::GetMyOtherOppnCollBub.

// FUNCTION: HYPERX 0x0047cb80
short PlanFocusArg::GetViewOnrsOppn2COB(EngineObject*, PlanNode*)
{
    const char cFocus = (char)g_pGameEngine->m_nViewedSlot;

    return (short)((((char)~cFocus & 1) | 2) * 3);
}

// The empty slot answers nought rather than the minus one the camera stores.

// FUNCTION: HYPERX 0x0047cba0
short PlanFocusArg::GetWatchedObject(EngineObject*, PlanNode*)
{
    int nSlot = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->m_nSecondaryTracked;

    if (nSlot == -1)
    {
        nSlot = 0;
    }

    return nSlot;
}

// The first thing the camera follows, answered the same way.

// FUNCTION: HYPERX 0x0047cbc0
short PlanFocusArg::GetTrackedObject(EngineObject*, PlanNode*)
{
    int nSlot = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->m_nPrimaryTracked;

    if (nSlot == -1)
    {
        nSlot = 0;
    }

    return nSlot;
}

// FUNCTION: HYPERX 0x0047cbe0
short PlanFocusArg::GetTrackedObjsTeammate(EngineObject*, PlanNode*)
{
    const int nTracked = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->m_nPrimaryTracked;

    return (short)TeammateSlot((short)nTracked);
}

// FUNCTION: HYPERX 0x0047cc10
short PlanFocusArg::GetViewOwnersGoal(EngineObject*, PlanNode*)
{
    return g_pGameEngine->GetPlayer(g_pGameEngine->m_nViewedSlot)->m_wTargetGoalSlot;
}

// The goalie of the side opposite the focus slot's, mirroring PlanArg::GetMyGoalie.

// FUNCTION: HYPERX 0x0047cc30
short PlanFocusArg::GetVwOwnrOppnGoalie(EngineObject*, PlanNode*)
{
    return (short)(((g_pGameEngine->m_nViewedSlot - 1) & 1) + GOALIE_SLOT_A);
}

// FUNCTION: HYPERX 0x0047cc50
short PlanFocusArg::GetViewOwnersTeammate(EngineObject*, PlanNode*)
{
    return (short)TeammateSlot(g_pGameEngine->m_nViewedSlot);
}

// FUNCTION: HYPERX 0x0047cc70
short PlanFocusArg::GetViewOnrsOppn1(EngineObject*, PlanNode*)
{
    const char cLocal = (char)g_pGameEngine->m_nViewedSlot;

    return (char)~cLocal & 1;
}

// FUNCTION: HYPERX 0x0047cc80
short PlanFocusArg::GetViewOnrsOppn2(EngineObject*, PlanNode*)
{
    const char cFocus = (char)g_pGameEngine->m_nViewedSlot;

    return ((char)~cFocus & 1) | 2;
}

// A slot of six or more, or no tracker, answers minus one.
// FUNCTION: HYPERX 0x0047cc90
short __cdecl PlanFocusArg::GetRokCarrier(EngineObject*, PlanNode*)
{
    PhysicsBody* const pBlock = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody();

    AttachPak* pTracked = pBlock->pAttach;

    if (pTracked != 0 && pTracked->nSlot < PLAYER_OBJECT_COUNT)
    {
        return pTracked->nSlot;
    }

    return -1;
}

// The Rok keeps its carrier slot itself; the last and previous carrier pronouns both answer it.
// FUNCTION: HYPERX 0x0047ccc0
short PlanFocusArg::GetLastRokCarrier(EngineObject*, PlanNode*)
{
    return (short)(g_pGameEngine->GetRok())->m_nCarrierSlot;
}

// One word from whatever the tracker points at, or minus one for nothing.
// FUNCTION: HYPERX 0x0047cce0
short __cdecl PlanFocusArg::GetRokCarrierCOB(EngineObject*, PlanNode*)
{
    PhysicsBody* const pBlock = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody();

    AttachPak* const pTracked = pBlock->pAttach;

    if (pTracked != 0)
    {
        return pTracked->nIndex;
    }

    return -1;
}

// A non-player or same-parity carrier answers minus one; otherwise three times its team mate's slot.
// FUNCTION: HYPERX 0x0047cd10
short __cdecl PlanFocusArg::GetNonRokCarryingOppn(EngineObject* pActor, PlanNode*)
{
    PhysicsBody* const pBlock = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody();

    AttachPak* const pTracked = pBlock->pAttach;

    if (pTracked != 0)
    {
        const short nTracked = pTracked->nSlot;

        if (nTracked < PLAYER_OBJECT_COUNT && (((unsigned char)pActor->GetSlot() ^ (unsigned char)nTracked) & 1) != 0)
        {
            return (short)(((nTracked + 2) % 4) * 3);
        }
    }

    return -1;
}
