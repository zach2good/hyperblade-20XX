#include "common/compat.h"
#include "planvm/gamebehaviorobject.h"
#include "gameengine.h"
#include "engine/stationaryplanrecord.h"
#include "planvm/planvm.h"
#include "graphics/meshbone.h"
#include "devicector.h"
#include "globals.h"
#include "net/netglobals.h"
#include "net/syncblock.h"
#include "objectclasses.h"
#include <windows.h>
#include "app/gameapp.h"
#include "rokobject.h"
#include "engine/level.h"

// Slot seventeen resets the object once its plan has been chosen.

#define STATIONARY_GROUPS 3
#define PLAN_NODES_MAX    0x100
#define GROUP_PRODUCT_MAX 0xffff

// The node counts and behaviour groups the network host sends, one record per stationary slot.
// GLOBAL: HYPERX 0x004ce6e4
extern StationaryPlanRecord g_aStationaryPlans[STATIONARY_SLOTS];

// Plan chains are counted to at most 256 nodes; four accumulators hold the addressing budget.
// FUNCTION: HYPERX 0x004415f0
int __stdcall Level::IndexStationaryObjects(void)
{
    int aAcc[4];
    int nNodes;

    int  nGroup = 0;
    int* pAcc   = aAcc;
    aAcc[0] = aAcc[1] = aAcc[2] = aAcc[3] = 0;
    g_nStationaryCount                    = 0;
    for (int nIndex = 0; nIndex < GODS_INVENTORY_LAST; nIndex++)
    {
        if (g_godsInventory[nIndex].nKind != 0)
        {
            for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
            {
                if ((short)nIndex == g_pGameEngine->m_ppObjects[i]->GetSlotWord())
                {
                    if (i != -1)
                    {
                        if (g_nStationaryCount >= STATIONARY_SLOTS)
                        {
                            g_nStationaryCount = 0;

                            g_gameApp.Error(
                                // STRING: HYPERX 0x004c19c0
                                "Too many stationary objects for the network buffers");

                            return 0;
                        }

                        g_aStationarySlots[g_nStationaryCount] = nIndex;

                        g_pGameEngine->m_ppSlots[(short)nIndex]->SelectAndActivatePlan();

                        if (g_pGameEngine->m_ppSlots[(short)nIndex]->m_nPlanCount != 1)
                        {
                            // STRING: HYPERX 0x004c1990
                            g_gameApp.Fatal("Static model %d:-%s- has more than one plan\n",
                                            nIndex,
                                            g_pGameEngine->m_ppSlots[(short)nIndex]
                                                ->m_pPlanStates->m_pszName);

                            return 0;
                        }

                        g_pGameEngine->m_ppSlots[(short)nIndex]->InstantiateAIPlan(0);

                        GameBehaviorObject* pNode = g_pGameEngine->m_ppSlots[(short)nIndex]->m_pPlanStates->m_pHead;

                        for (nNodes = 1; nNodes < PLAN_NODES_MAX; nNodes++)
                        {
                            pNode = pNode->m_pStandby;

                            if (pNode == 0)
                            {
                                break;
                            }
                        }

                        if ((*pAcc >> 16) * nNodes > GROUP_PRODUCT_MAX)
                        {
                            pAcc++;
                            nGroup++;

                            if (nGroup > STATIONARY_GROUPS)
                            {
                                g_gameApp.Error(
                                    // STRING: HYPERX 0x004c195c
                                    "Too many behaviors for static objects (for network)");

                                return 0;
                            }
                        }

                        *pAcc = (*pAcc + 1) * nNodes - 1;

                        g_aStationaryPlans[g_nStationaryCount].nNodes = nNodes;
                        g_aStationaryPlans[g_nStationaryCount].nGroup = nGroup;

                        g_nStationaryCount++;
                    }

                    break;
                }
            }
        }
    }

    return 1;
}

// Checks that both ends of a network match built the same arena.
// FUNCTION: HYPERX 0x00442140
int __cdecl Level::ValidateArenaObjects(SyncBlocks*, SyncBlocks* pRemote)
{
    const SyncSettingsBlock* pLevel = pRemote->Settings();

    if (pLevel->nSlotCount != (unsigned char)g_nStationaryCount)
    {
        g_gameApp.Error(
            // STRING: HYPERX 0x004c1bb8
            "%s Arena object count has %d instead of %d\nChange arenas and teams, then try again\n",
            // STRING: HYPERX 0x004c1208
            "Network connection failed because HOST and CLIENT had different asset files.\n",
            g_nStationaryCount,
            pLevel->nSlotCount);

        return 0;
    }

    for (int nIndex = 0; nIndex < g_nStationaryCount; nIndex++)
    {
        if (pLevel->aSlots[nIndex] != (unsigned char)g_aStationarySlots[nIndex])
        {
            g_gameApp.Error(
                // STRING: HYPERX 0x004c1b90
                "%s REF object %d is %d instead of %d\n",
                // STRING: HYPERX 0x004c1208
                "Network connection failed because HOST and CLIENT had different asset files.\n",
                nIndex,
                g_aStationarySlots[nIndex],
                pLevel->aSlots[nIndex]);

            return 0;
        }
    }

    return 1;
}
