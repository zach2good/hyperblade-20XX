#include "common/compat.h"
#include "globals.h"
#include "session.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "engine/gameengine.h"
#include "planvm/planvm.h"
#include "engine/objectclasses.h"
#include "net/gamesession.h"
#include "engine/messagequeue.h"
#include "netglobals.h"
#include "app/gameapp.h"
#include "cplayer.h"
#include "engine/rokobject.h"
#include "sound/audiomgr.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "engine/teamsblock.h"
#include "engine/teamblock.h"
#include <string.h>
#include <windows.h>
#include <stdio.h>
#include "engine/engineobject.h"
#include "syncblock.h"
#include "netsetupstate.h"
#include <io.h>
#include "common/fileobj.h"
#include "common/parseutil.h"
#include "sound/soundids.h"
#include "engine/objectphase.h"
#include <SDL3/SDL.h>
#include <cstdlib>

#define SCAT_TOKEN_MAX 0x100

#define FIND_FAILED (-1)

#define SYNC_NAME_MAX 16

#define SLOT_GROUP 6

#define VERSION_MAGIC 0x934c5347

#define SLOT_PAD 0xff

#define PLAN_PROBE 0x400

#define PLAN_LIMIT 0x29

extern const char* g_apNetworkSlotNames[16];

namespace SceneInfo
{

static inline char* TeamName(const TeamsBlock* pInfo, short nIndex);
static inline char* ArenaName(const TeamsBlock* pInfo, short nIndex);

} // namespace SceneInfo

#define SESSION_LAG_DEFAULT_MS 4

extern int g_anNetworkSlotObject[16];

#define DISCONNECT_HOLD_MS 0x1388

#define ASSET_NAME_MAX 16

#define PERIOD_SCALE 1000

// STRING: HYPERX 0x004c1208
#define ASSETS_DIFFER "Network connection failed because HOST and CLIENT had different asset files.\n"

// The undo walk after a failed allocation runs one slot past the last success.
// FUNCTION: HYPERX 0x00440850
GameSession::GameSession(HWND hDlgIn, IDirectPlay* pLobby)
{
    memset(&sessionDesc, 0, sizeof(sessionDesc));

    hDlg               = hDlgIn;
    pDPlay             = pLobby;
    bHosting           = 0;
    nRemoteCount       = 0;
    pLeadPlayer        = 0;
    nPendingDisconnect = 0;
    nLagMs             = SESSION_LAG_DEFAULT_MS;
    nAssetsArrived     = 0;

    for (int nSlot = 0; nSlot < SESSION_PLAYER_SLOTS; nSlot++)
    {
        CPlayer* const pPlayer = new CPlayer(this);

        apPlayers[nSlot] = pPlayer;

        if (pPlayer == 0)
        {
            // STRING: HYPERX 0x004c167c
            g_gameApp.Error("Memory allocation (new) failed in CDirectPlay() constructor");

            for (int i = 0; i <= nSlot; i++)
            {
                if (apPlayers[i] != 0)
                {
                    apPlayers[i] = (CPlayer*)apPlayers[i]->DestroyViaVtable();
                }
            }

            return;
        }

        pPlayer->m_nSlot = nSlot;
    }
}

// GLOBAL: HYPERX 0x004ce924
extern HANDLE g_hPacketCarrierEvent;

// FUNCTION: HYPERX 0x00440940
GameSession::~GameSession()
{
    if (pDPlay != 0)
    {
        pDPlay->Release();
    }

    pDPlay = 0;

    for (int nSlot = 0; nSlot < SESSION_PLAYER_SLOTS; nSlot++)
    {
        if (apPlayers[nSlot] != 0)
        {
            apPlayers[nSlot] = (CPlayer*)apPlayers[nSlot]->DestroyViaVtable();
        }
    }
}

// The wait between retries does not wait: it spins only once the deadline has already passed.
// FUNCTION: HYPERX 0x00440980
int GameSession::OpenSession(DWORD dwFlags, const DPSESSIONDESC* pDesc)
{
    sessionDesc = *pDesc;

    sessionDesc.dwFlags      = dwFlags;
    sessionDesc.dwMaxPlayers = 0x7f;

    if (dwFlags == DPOPEN_CREATE)
    {
        bHosting = 1;

        sessionDesc.guidSession = g_guidNetApplication;

        sessionDesc.dwSession        = 0;
        sessionDesc.dwCurrentPlayers = 0;
    }

    int nResult = pDPlay->Open(&sessionDesc);

    for (int i = 0; i < 2 && nResult != 0; i++)
    {
        const unsigned int dwStart = (unsigned int)SDL_GetTicks();

        while (dwStart + 200 < (unsigned int)SDL_GetTicks())
        {
        }

        nResult = pDPlay->Open(&sessionDesc);
    }

    if (nResult != 0)
    {
        // STRING: HYPERX 0x004c0e48
        g_gameApp.Error("Network connection failed.\nPlease try again.\n");

        return 0;
    }

    nRemoteCount = 0;

    return 1;
}

// FUNCTION: HYPERX 0x00440a60
int GameSession::AssignPlayerSlot(unsigned int nFlags, PlayerSlotName* pName)
{
    PlayerState* pAssign;

    const unsigned int nSlot   = nFlags & PLAYER_NAME_SLOT_MASK;
    CPlayer* const     pPlayer = apPlayers[nSlot];

    if (pPlayer != 0 && (pAssign = pPlayer->pState) != 0)
    {
        if (((nFlags & PLAYER_NAME_HOST) == 0 && bHosting != 0) || ((nFlags & PLAYER_NAME_HOST) != 0 && bHosting == 0))
        {
            // STRING: HYPERX 0x004c173c
            g_gameApp.Error("Invalid remote host/join flag in CreatePlayer()");

            return 0;
        }

        if (pAssign->nSlotFlags != 0)
        {
            // STRING: HYPERX 0x004c16e8
            g_gameApp.Error("Sorry, -%s- position has just been filled by\nanother player.  Try another position\n",
                            g_apNetworkSlotNames[nSlot]);

            return 0;
        }

        apPlayers[nSlot]->pState->nPlayerId = DPID_UNKNOWN;
        pPlayer->pState->nConnected         = 0;
        pPlayer->pState->nHuman             = (nFlags & PLAYER_NAME_HUMAN) != 0;
        pPlayer->pState->nObjectSlot        = g_anNetworkSlotObject[nSlot];

        strcpy(pPlayer->pState->slotName.GetWireName(), pName->GetWireName());

        pPlayer->pState->bLocal        = (nFlags & PLAYER_NAME_LOCAL) != 0;
        pPlayer->pState->bHost         = (nFlags & PLAYER_NAME_HOST) != 0;
        pPlayer->pState->nSlotFlags    = nSlot;
        pPlayer->pState->nReady        = 0;
        pPlayer->pState->nActive       = 0;
        pPlayer->pState->nDisconnected = 0;

        if (pPlayer->pState->bLocal != 0)
        {
            pLeadPlayer = pPlayer;
        }

        pPlayer->ResetState();

        return 1;
    }

    // STRING: HYPERX 0x004c16b8
    g_gameApp.Error("Player[%d] is set to NULL in CreatePlayer()\n", nSlot);

    return 0;
}

// Slot fifteen is " Packet Carrier"; the leading space keeps it first in a sorted list.

// FUNCTION: HYPERX 0x00440be0
unsigned int GameSession::CreatePacketCarrier()
{
    apPlayers[SESSION_LOCAL_PLAYER]->InitDirectPlayParams();

    apPlayers[SESSION_LOCAL_PLAYER]->ResetState();

    // STRING: HYPERX 0x004c17ac
    strcpy(apPlayers[SESSION_LOCAL_PLAYER]->pState->slotName.GetWireName(), " Packet Carrier");

    g_hPacketCarrierEvent = 0;

    if (pDPlay->CreatePlayer(&apPlayers[SESSION_LOCAL_PLAYER]->pState->nPlayerId, " Packet Carrier", " Packet Carrier", &g_hPacketCarrierEvent) != 0)
    {
        // STRING: HYPERX 0x004c0e48
        g_gameApp.Error("Network connection failed.\nPlease try again.\n");

        return 0;
    }

    if (g_hPacketCarrierEvent != 0)
    {
        if (apPlayers[SESSION_LOCAL_PLAYER]->StartNetworkThread(g_hPacketCarrierEvent) != 0)
        {
            return 1;
        }
    }

    apPlayers[SESSION_LOCAL_PLAYER]->StopNetworkThread();

    // STRING: HYPERX 0x004c176c
    g_gameApp.Error("Failed to start thread with event handle from CreatePlayer()");

    return 0;
}

// The plan starts only when the object's current plan is named Human, compared with _strcmpi.

// FUNCTION: HYPERX 0x00440e90
void GameSession::ActivatePlayerSlot(int nSlot, unsigned int bHost)
{
    EngineObject* const pObject = g_pGameEngine->m_ppSlots[(short)g_anNetworkSlotObject[nSlot]];

    if (pObject == 0)
    {
        // STRING: HYPERX 0x004c1838
        g_gameApp.Error("Trying to take over invalid model %d", nSlot);

        return;
    }

    g_pGameSession->apPlayers[nSlot]->pState->bHost       = bHost;
    g_pGameSession->apPlayers[nSlot]->pState->nConnected  = 0;
    g_pGameSession->apPlayers[nSlot]->pState->bLocal      = 0;
    g_pGameSession->apPlayers[nSlot]->pState->nObjectSlot = g_anNetworkSlotObject[nSlot];
    g_pGameSession->apPlayers[nSlot]->pState->nReady      = 1;
    g_pGameSession->apPlayers[nSlot]->pState->nActive     = 1;
    g_pGameSession->apPlayers[nSlot]->pState->nHuman      = 0;

    g_pGameSession->apPlayers[nSlot]->ResetState();

    g_pGameSession->apPlayers[nSlot]->AssignObject(g_anNetworkSlotObject[nSlot]);

    pObject->GetPhysicsBody()->ResetDynamics();

    pObject->m_nPhase = OBJECT_PHASE_LOCAL;

    if (pObject->GetPlanState() != 0)
    {
        const char* const pszName = pObject->GetPlanState()->m_pszName;

        // STRING: HYPERX 0x004bb340
        if (SDL_strcasecmp(pszName, "Human") == 0)
        {
            g_pGameEngine->m_ppSlots[(short)g_anNetworkSlotObject[nSlot]]->SelectAndActivatePlan();
        }
    }
}

// The second slot chain has no final arm; in the original its fall-through reads an unassigned local.

// FUNCTION: HYPERX 0x00441000
void GameSession::HandlePlayerDisconnect(int nIndex)
{
    char szMessage[0x100];
    int  nOther;
    int  nSwap = 0;
    int  i;

    const int nHost  = g_pGameSession->pLeadPlayer->pState->nSlotFlags;
    int       nFound = nHost;

    if (nIndex == 2)
    {
        nOther = 4;
    }
    else if (nIndex == 4)
    {
        nOther = 2;
    }
    else if (nIndex == 1)
    {
        nOther = 3;
    }
    else if (nIndex == 3)
    {
        nOther = 1;
    }
    else
    {
        g_gameApp.Fatal(
            // STRING: HYPERX 0x004c1860
            "Invalid player position %d in ReplaceClient()\n",
            nIndex);

        return;
    }

    sprintf(szMessage,
            // STRING: HYPERX 0x004c1890
            "%s has left the game",
            g_pGameSession->apPlayers[nIndex]->pState->slotName.szName);

    g_pMessageQueue->Push(szMessage, 0, DISCONNECT_HOLD_MS);
    g_pAudioMgr->PlaySpecific(SOUND_INJURE1);

    g_pGameSession->apPlayers[nIndex]->pState->nDisconnected = 0;
    g_pGameSession->apPlayers[nIndex]->pState->bLocal        = 0;
    g_pGameSession->apPlayers[nIndex]->pState->nHuman        = 0;

    if (g_pGameSession->apPlayers[nIndex]->pState->bHost != 0)
    {
        for (i = 0; i < nHost; i++)
        {
            if (g_pGameSession->apPlayers[i]->pState->bLocal != 0 && i != nIndex)
            {
                g_pGameSession->apPlayers[i]->pState->bHost = 1;
                nFound                                      = i;
                break;
            }
        }

        if (nFound == nHost)
        {
            for (i = 0; i < SESSION_PLAYER_COUNT; i++)
            {
                if (g_pGameSession->apPlayers[i]->pState->bHost != 0)
                {
                    ActivatePlayerSlot(i, 1);
                }
            }

            bHosting                                        = 1;
            g_pGameSession->apPlayers[nHost]->pState->bHost = 1;
        }

        if (nFound == 2)
        {
            nSwap = 4;
        }
        else if (nFound == 4)
        {
            nSwap = 2;
        }
        else if (nFound == 1)
        {
            nSwap = 3;
        }
        else if (nFound == 3)
        {
            nSwap = 1;
        }

        if (g_pGameSession->apPlayers[nSwap]->pState->nHuman == 0)
        {
            g_pGameSession->apPlayers[nSwap]->pState->bHost = 1;
        }
    }

    g_pGameSession->apPlayers[nIndex]->pState->bHost = 0;

    if (g_pGameSession->apPlayers[nOther]->pState->nHuman != 0)
    {
        if (nHost == nOther)
        {
            ActivatePlayerSlot(nIndex, nHost == nFound);

            return;
        }

        if (g_pGameSession->apPlayers[nOther]->pState->bHost == 0)
        {
            return;
        }

        g_pGameSession->apPlayers[nIndex]->pState->bHost = 1;

        return;
    }

    if (nHost == nFound)
    {
        ActivatePlayerSlot(nIndex, 1);
        ActivatePlayerSlot(nOther, 1);

        return;
    }

    g_pGameSession->apPlayers[nIndex]->pState->bHost  = 1;
    g_pGameSession->apPlayers[nIndex]->pState->bLocal = 0;
    g_pGameSession->apPlayers[nIndex]->pState->nHuman = 0;
    g_pGameSession->apPlayers[nOther]->pState->bHost  = 1;
    g_pGameSession->apPlayers[nOther]->pState->bLocal = 0;
    g_pGameSession->apPlayers[nOther]->pState->nHuman = 0;
}

// Answers whether there was a session, not whether any command was handled.

// FUNCTION: HYPERX 0x00441360
int GameSession::PumpNetworkCommands()
{
    CPlayer**    ppPlayer;
    PlayerState* pState;
    int          nIndex;

    if (g_bSessionActive == 0)
    {
        return 0;
    }

    if (nPendingDisconnect != 0)
    {
        for (nIndex = 0, ppPlayer = &apPlayers[0]; nIndex < 16; nIndex++, ppPlayer++)
        {
            if ((*ppPlayer)->pState->nDisconnected != 0)
            {
                HandlePlayerDisconnect(nIndex);
            }
        }

        nPendingDisconnect = 0;
    }

    for (nIndex = 1, ppPlayer = &apPlayers[1]; nIndex < 15; nIndex++, ppPlayer++)
    {
        pState = (*ppPlayer)->pState;

        if (pState->nSlotFlags != 0 && pState->nConnected == 0)
        {
            if (pState->nObjectSlot == SLOT_GOD)
            {
                (*ppPlayer)->NetUpdateMultiObject(nIndex);
            }
            else
            {
                (*ppPlayer)->NetUpdateObject((unsigned char)nIndex);
            }
        }
    }

    return 1;
}

// FUNCTION: HYPERX 0x00441400
int GameSession::AreAllSlotsReady()
{
    PlayerState* pState;

    CPlayer* pPlayer = apPlayers[1];

    if (pPlayer != 0)
    {
        pState = pPlayer->pState;

        if (pState->nConnected == 0 || pState->bLocal == 0 || pState->nReady != 0)
        {
            pPlayer = apPlayers[3];

            if (pPlayer != 0)
            {
                pState = pPlayer->pState;

                if (pState->nConnected == 0 || pState->bLocal == 0 || pState->nReady != 0)
                {
                    pPlayer = apPlayers[2];

                    if (pPlayer != 0)
                    {
                        pState = pPlayer->pState;

                        if (pState->nConnected == 0 || pState->bLocal == 0 || pState->nReady != 0)
                        {
                            pPlayer = apPlayers[4];

                            if (pPlayer != 0)
                            {
                                pState = pPlayer->pState;

                                if (pState->nConnected == 0 || pState->bLocal == 0 || pState->nReady != 0)
                                {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

// An empty slot zero skips the whole loop and leaves the rest attached.

// FUNCTION: HYPERX 0x004414b0
GameSession* GameSession::Destroy()
{
    int nPlayerId;
    int hr;

    g_bSessionActive  = 0;
    g_nSessionState   = 0;
    g_nDPlayLastError = -1;

    if (apPlayers[SESSION_LOCAL_PLAYER] != 0)
    {
        apPlayers[SESSION_LOCAL_PLAYER]->SendDropPlayer(DPID_ALLPLAYERS);

        nPlayerId = apPlayers[SESSION_LOCAL_PLAYER]->pState->nPlayerId;

        apPlayers[SESSION_LOCAL_PLAYER]->DetachObject();

        if (nPlayerId != DPID_UNKNOWN && pDPlay != 0)
        {
            hr = pDPlay->DestroyPlayer(nPlayerId);

            if (hr != 0)
            {
                // STRING: HYPERX 0x004c18c8
                g_gameApp.Error("pIDC->DestroyPlayer() failed with %s", DirectPlay::GetErrorString(hr));
            }
        }
    }

    CPlayer** ppPlayer = &apPlayers[0];

    if (apPlayers[0] != 0)
    {
        for (int nSlot = 0; nSlot < 16; nSlot++)
        {
            if (*ppPlayer != 0)
            {
                (*ppPlayer)->DetachObject();

                *ppPlayer = (CPlayer*)(*ppPlayer)->DestroyViaVtable();
            }

            ppPlayer++;
        }
    }

    if (pDPlay != 0)
    {
        hr = pDPlay->Close();

        if (hr != 0)
        {
            // STRING: HYPERX 0x004c18a8
            g_gameApp.Error("pIDC->Close() failed with %s", DirectPlay::GetErrorString(hr));
        }
    }

    delete this;

    return 0;
}

// The settings are narrowed to their packet width, so both machines agree on a truncated value.
// FUNCTION: HYPERX 0x004417d0
int GameSession::BuildSyncPacket(SyncBlocks* pBlocks, SyncBlocks*)
{
    PathString         strPath(MAX_PATH);
    char               szName[SYNC_NAME_MAX];
    SyncSettingsBlock* pSettings;
    int                nWriteTime;
    int                nSize;

    int nIndex = 0;

    if (g_pGameEngine == 0 || g_pGameEngine->m_pTeamTable == 0 || g_pGameEngine->m_pTeamTable->GetTeamCount() == 0)
    {
        // STRING: HYPERX 0x004c19f4
        g_gameApp.Error("%s could not find Scene Manager info\n", ASSETS_DIFFER);

        return 0;
    }

    pSettings = pBlocks->Settings();

    pSettings->nKind = ASSET_TYPE_SETTINGS;

    pSettings->nCount = (unsigned char)((g_nStationaryCount + 5) / SLOT_GROUP + SLOT_GROUP);

    pSettings->nVersion = VERSION_MAGIC;

    pSettings->nDifficulty = (unsigned char)g_pGameEngine->GetDifficultyLevel();
    const int nBrutality   = g_pGameEngine->m_nBrutality;
    pSettings->nBrutality  = (unsigned char)nBrutality;

    pSettings->nPeriod = (short)(g_pGameEngine->m_nPeriodLengthMs / PERIOD_SCALE);

    pSettings->nArena     = (unsigned char)g_nDromeArena;
    pSettings->nTeamOne   = (unsigned char)g_nDromeTeamHome;
    pSettings->nTeamTwo   = (unsigned char)g_nDromeTeamAway;
    pSettings->nSlotCount = (unsigned char)g_nStationaryCount;

    while (nIndex < g_nStationaryCount)
    {
        pSettings->aSlots[nIndex] = (unsigned char)g_aStationarySlots[nIndex];

        nIndex++;
    }

    const int nPadTo = ((g_nStationaryCount + 5) / SLOT_GROUP) * SLOT_GROUP;

    while (nIndex < nPadTo)
    {
        pSettings->aSlots[nIndex] = SLOT_PAD;

        nIndex++;
    }

    g_pGameEngine->SetDifficultyLevel(pSettings->nDifficulty);
    g_pGameEngine->SetBrutality(pSettings->nBrutality);
    g_pGameEngine->SetPeriodLengthMs(pSettings->nPeriod * PERIOD_SCALE);

    // STRING: HYPERX 0x004c1b70
    strPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "ENUMIDS.H"));

    if (GameSession::GetFileInfo(strPath.m_str.m_pchData, &nWriteTime, &nSize) == 0)
    {
        // STRING: HYPERX 0x004c1b50
        g_gameApp.Error("%s could not find ENUMIDS.H\n", ASSETS_DIFFER);

        return 0;
    }

    pSettings->enumIds.wTimeHigh = (short)(nWriteTime >> 16);
    pSettings->enumIds.wTimeLow  = (short)nWriteTime;
    pSettings->enumIds.wSize     = (short)nSize;

    // STRING: HYPERX 0x004c1b44
    strPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "TEAMS.CAT"));

    if (GameSession::GetFileInfo(strPath.m_str.m_pchData, &nWriteTime, &nSize) == 0)
    {
        // STRING: HYPERX 0x004c1b24
        g_gameApp.Error("%s could not find TEAMS.CAT\n", ASSETS_DIFFER);

        return 0;
    }

    pSettings->teams.wTimeHigh = (short)(nWriteTime >> 16);
    pSettings->teams.wTimeLow  = (short)nWriteTime;
    pSettings->teams.wSize     = (short)nSize;

    // STRING: HYPERX 0x004c1b18
    strPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "PLANS.CAT"));

    if (GameSession::GetFileInfo(strPath.m_str.m_pchData, &nWriteTime, &nSize) == 0)
    {
        // STRING: HYPERX 0x004c1af8
        g_gameApp.Error("%s could not find PLANS.CAT\n", ASSETS_DIFFER);

        return 0;
    }

    pSettings->plans.wTimeHigh = (short)(nWriteTime >> 16);
    pSettings->plans.wTimeLow  = (short)nWriteTime;
    pSettings->plans.wSize     = (short)nSize;

    // STRING: HYPERX 0x004c1aec
    strPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "COMMON.CFG"));

    if (GameSession::GetFileInfo(strPath.m_str.m_pchData, &nWriteTime, &nSize) == 0)
    {
        // STRING: HYPERX 0x004c1acc
        g_gameApp.Error("%s could not find COMMON.CFG\n", ASSETS_DIFFER);

        return 0;
    }

    pSettings->common.wTimeHigh = (short)(nWriteTime >> 16);
    pSettings->common.wTimeLow  = (short)nWriteTime;
    pSettings->common.wSize     = (short)nSize;

    SyncCatalogBlock* pBlock = pBlocks->Teams();

    pBlock->nKind  = ASSET_TYPE_TEAMS;
    pBlock->nCount = (char)g_pGameEngine->m_pTeamTable->GetTeamCount();

    for (nIndex = 0; nIndex < pBlock->nCount; nIndex++)
    {
        strPath.m_str.AssignFromString(
            String::Concat(g_gameApp.m_strExeDir.m_str, g_pGameEngine->m_pTeamTable->GetTeamPath(nIndex)));

        if (GameSession::GetFileInfo(strPath.m_str.m_pchData, &nWriteTime, &nSize) != 0)
        {
            pBlock->aStamps[nIndex].wTimeHigh = (short)(nWriteTime >> 16);
            pBlock->aStamps[nIndex].wTimeLow  = (short)nWriteTime;
            pBlock->aStamps[nIndex].wSize     = (short)nSize;
        }
        else
        {
            // STRING: HYPERX 0x004c1aac
            g_gameApp.Error("%s could not find team file %s\n", ASSETS_DIFFER, g_pGameEngine->m_pTeamTable->GetTeamPath(nIndex));

            return 0;
        }
    }

    pBlock = pBlocks->Arenas();

    pBlock->nKind  = ASSET_TYPE_ARENAS;
    pBlock->nCount = (char)g_pGameEngine->m_pTeamTable->GetTeamCount();

    for (nIndex = 0; nIndex < pBlock->nCount; nIndex++)
    {
        strPath.m_str.AssignFromString(
            String::Concat(g_gameApp.m_strExeDir.m_str, g_pGameEngine->m_pTeamTable->GetTeamEnvironment(nIndex)));

        if (GameSession::GetFileInfo(strPath.m_str.m_pchData, &nWriteTime, &nSize) != 0)
        {
            pBlock->aStamps[nIndex].wTimeHigh = (short)(nWriteTime >> 16);
            pBlock->aStamps[nIndex].wTimeLow  = (short)nWriteTime;
            pBlock->aStamps[nIndex].wSize     = (short)nSize;
        }
        else
        {
            // STRING: HYPERX 0x004c1a74
            g_gameApp.Error("%s could not find arena file %s.ENV\n",
                            ASSETS_DIFFER,
                            g_pGameEngine->m_pTeamTable->GetTeamEnvironment(nIndex));

            return 0;
        }
    }

    pBlock = pBlocks->Plans();

    pBlock->nKind = ASSET_TYPE_PLANS;

    const int nPlans = GameSession::GetPlansEntry("PLANS.CAT", szName, PLAN_PROBE);

    pBlock->nCount = (unsigned char)nPlans;

    if ((unsigned char)nPlans <= 0 || (unsigned char)nPlans > PLAN_LIMIT)
    {
        // STRING: HYPERX 0x004c1a1c
        g_gameApp.Error("%s corrupted or missing PLANS.CAT file (%d lines found)\n",
                        ASSETS_DIFFER,
                        (unsigned char)nPlans);

        return 0;
    }

    for (nIndex = 0; nIndex < pBlock->nCount; nIndex++)
    {
        GameSession::GetPlansEntry("PLANS.CAT", szName, nIndex);

        strPath.m_str.AssignFromString(
            String::Concat(String::Concat(String::Concat(g_gameApp.m_strExeDir.m_str,
                                                         // STRING: HYPERX 0x004c1aa4
                                                         "PLANS\\"),
                                          szName),
                           // STRING: HYPERX 0x004c1a9c
                           ".PBN"));

        if (GameSession::GetFileInfo(strPath.m_str.m_pchData, &nWriteTime, &nSize) != 0)
        {
            pBlock->aStamps[nIndex].wTimeHigh = (short)(nWriteTime >> 16);
            pBlock->aStamps[nIndex].wTimeLow  = (short)nWriteTime;
            pBlock->aStamps[nIndex].wSize     = (short)nSize;
        }
        else
        {
            // STRING: HYPERX 0x004c1a58
            g_gameApp.Error("%s could not find %s.PBN\n", ASSETS_DIFFER, szName);

            return 0;
        }
    }

    return 1;
}

// Opens a find handle only to read the write time and size; a missing file answers zero.
// FUNCTION: HYPERX 0x00441f00
int GameSession::GetFileInfo(const char* pszPath, int* pnWriteTime, int* pnSize)
{
    struct _finddata_t data;

    const intptr_t nHandle = _findfirst(pszPath, &data);

    if (nHandle != FIND_FAILED)
    {
        *pnWriteTime = (int)data.time_write;
        *pnSize      = data.size;

        _findclose(nHandle);

        return 1;
    }

    return 0;
}

#define SCAT_NAME_MAX 0xf

#define SCAT_COMMENT '#'

// Fills the name with eight question marks first; a hash starts a comment line.
// FUNCTION: HYPERX 0x00441f60
int GameSession::GetPlansEntry(const char* pszFileName, char* pszName, int nWanted)
{
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData, MAX_PATH);
    OwnedFile  file;
    char       szToken[SCAT_TOKEN_MAX];
    char*      pLine;

    int nSeen = 0;

    // STRING: HYPERX 0x004c1b84
    strcpy(pszName, "????????");

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0)
    {
        while (file.m_file.GetNextLine(&pLine) > 0)
        {
            if (pLine == 0)
            {
                continue;
            }

            // STRING: HYPERX 0x004c1b7c
            if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r.", (short)strlen(pLine), &pLine) == 0)
            {
                continue;
            }

            if (szToken[0] == SCAT_COMMENT)
            {
                continue;
            }

            if (nSeen == nWanted)
            {
                strncpy(pszName, szToken, SCAT_NAME_MAX);

                pszName[SCAT_NAME_MAX] = 0;

                file.m_file.CloseAndTest();

                return nSeen + 1;
            }

            nSeen++;
        }

        file.m_file.CloseAndTest();
    }

    return nSeen;
}

// The host's difficulty, brutality, period, arena and teams win; the period arrives in seconds.

// FUNCTION: HYPERX 0x004421d0
int GameSession::ValidateAndApplyAssets(SyncBlocks* pLocal, SyncBlocks* pRemote)
{
    char               szName[ASSET_NAME_MAX];
    TeamsBlock*        pInfo;
    SyncSettingsBlock* pRemoteSettings;
    SyncCatalogBlock*  pRemoteTeams;
    SyncCatalogBlock*  pRemoteArenas;
    SyncCatalogBlock*  pRemotePlans;
    SyncSettingsBlock* pLocalSettings;
    SyncCatalogBlock*  pLocalTeams;
    SyncCatalogBlock*  pLocalArenas;
    SyncCatalogBlock*  pLocalPlans;
    int                nIndex;

    if (pLocal == 0 || pRemote == 0 || (pRemoteSettings = pRemote->Settings()) == 0 || (pRemoteTeams = pRemote->Teams()) == 0 || (pRemoteArenas = pRemote->Arenas()) == 0 || (pRemotePlans = pRemote->Plans()) == 0 || (pLocalSettings = pLocal->Settings()) == 0 || (pLocalTeams = pLocal->Teams()) == 0 || (pLocalArenas = pLocal->Arenas()) == 0 || (pLocalPlans = pLocal->Plans()) == 0)
    {
        // STRING: HYPERX 0x004c1c0c
        g_gameApp.Error("%s asset list is NULL\n", ASSETS_DIFFER);

        return 0;
    }
    else if (pRemoteSettings->IsEmpty() || pRemoteTeams->IsEmpty() || pRemoteArenas->IsEmpty() || pRemotePlans->IsEmpty() || pLocalSettings->IsEmpty() || pLocalTeams->IsEmpty() || pLocalArenas->IsEmpty() || pLocalPlans->IsEmpty())
    {
        // STRING: HYPERX 0x004c1c24
        g_gameApp.Error("%s asset list is empty\n", ASSETS_DIFFER);

        return 0;
    }
    else if (g_pGameEngine == 0 || (pInfo = g_pGameEngine->m_pTeamTable) == 0 || pInfo->GetTeamCount() == 0)
    {
        // STRING: HYPERX 0x004c19f4
        g_gameApp.Error("%s could not find Scene Manager info\n", ASSETS_DIFFER);

        return 0;
    }
    else
    {
        if (pLocalSettings->nVersion != pRemoteSettings->nVersion)
        {
            // STRING: HYPERX 0x004c1e1c
            g_gameApp.Error("%s incorrect version (need %X, have %X)\n",
                            ASSETS_DIFFER,
                            pRemoteSettings->nVersion,
                            pLocalSettings->nVersion);

            return 0;
        }

        if (g_bSkipAssetValidation == 0)
        {
            if (pLocalSettings->enumIds.wTimeLow != pRemoteSettings->enumIds.wTimeLow || pLocalSettings->enumIds.wTimeHigh != pRemoteSettings->enumIds.wTimeHigh || pLocalSettings->enumIds.wSize != pRemoteSettings->enumIds.wSize)
            {
                // STRING: HYPERX 0x004c1d3c
                g_gameApp.Error("%s file ENUMIDS.H does not match\nhave=%X,%X\nneed=%X,%X",
                                ASSETS_DIFFER,
                                (pLocalSettings->enumIds.wTimeLow << 16) + pLocalSettings->enumIds.wTimeHigh,
                                pLocalSettings->enumIds.wSize,
                                (pRemoteSettings->enumIds.wTimeLow << 16) + pRemoteSettings->enumIds.wTimeHigh,
                                pRemoteSettings->enumIds.wSize);

                return 0;
            }

            if (pLocalSettings->teams.wTimeLow != pRemoteSettings->teams.wTimeLow || pLocalSettings->teams.wTimeHigh != pRemoteSettings->teams.wTimeHigh || pLocalSettings->teams.wSize != pRemoteSettings->teams.wSize)
            {
                // STRING: HYPERX 0x004c1d74
                g_gameApp.Error("%s file TEAMS.CAT does not match\nhave=%X,%X\nneed=%X,%X",
                                ASSETS_DIFFER,
                                (pLocalSettings->teams.wTimeLow << 16) + pLocalSettings->teams.wTimeHigh,
                                pLocalSettings->teams.wSize,
                                (pRemoteSettings->teams.wTimeLow << 16) + pRemoteSettings->teams.wTimeHigh,
                                pRemoteSettings->teams.wSize);

                return 0;
            }

            if (pLocalSettings->plans.wTimeLow != pRemoteSettings->plans.wTimeLow || pLocalSettings->plans.wTimeHigh != pRemoteSettings->plans.wTimeHigh || pLocalSettings->plans.wSize != pRemoteSettings->plans.wSize)
            {
                // STRING: HYPERX 0x004c1dac
                g_gameApp.Error("%s file PLANS.CAT does not match\nhave=%X,%X\nneed=%X,%X",
                                ASSETS_DIFFER,
                                (pLocalSettings->plans.wTimeLow << 16) + pLocalSettings->plans.wTimeHigh,
                                pLocalSettings->plans.wSize,
                                (pRemoteSettings->plans.wTimeLow << 16) + pRemoteSettings->plans.wTimeHigh,
                                pRemoteSettings->plans.wSize);

                return 0;
            }

            if (pLocalSettings->common.wTimeLow != pRemoteSettings->common.wTimeLow || pLocalSettings->common.wTimeHigh != pRemoteSettings->common.wTimeHigh || pLocalSettings->common.wSize != pRemoteSettings->common.wSize)
            {
                // STRING: HYPERX 0x004c1de4
                g_gameApp.Error("%s file COMMON.CFG does not match\nhave=%X,%X\nneed=%X,%X",
                                ASSETS_DIFFER,
                                (pLocalSettings->common.wTimeLow << 16) + pLocalSettings->common.wTimeHigh,
                                pLocalSettings->common.wSize,
                                (pRemoteSettings->common.wTimeLow << 16) + pRemoteSettings->common.wTimeHigh,
                                pRemoteSettings->common.wSize);

                return 0;
            }
        }

        if (g_bSkipAssetValidation == 0)
        {
            if (pLocalTeams->nCount != pRemoteTeams->nCount)
            {
                // STRING: HYPERX 0x004c1d0c
                g_gameApp.Error("%s number of TEAM (*.HTM) files does not match\n", ASSETS_DIFFER);

                return 0;
            }

            for (nIndex = 0; nIndex < pLocalTeams->nCount; nIndex++)
            {
                if (pRemoteTeams->aStamps[nIndex].wTimeLow != pLocalTeams->aStamps[nIndex].wTimeLow || pRemoteTeams->aStamps[nIndex].wTimeHigh != pLocalTeams->aStamps[nIndex].wTimeHigh || pRemoteTeams->aStamps[nIndex].wSize != pLocalTeams->aStamps[nIndex].wSize)
                {
                    // STRING: HYPERX 0x004c1c74
                    g_gameApp.Error("%s file %d:%s does not match\nhave=%X,%X\nneed=%X,%X",
                                    ASSETS_DIFFER,
                                    nIndex,
                                    SceneInfo::TeamName(pInfo, (short)nIndex),
                                    (pLocalTeams->aStamps[nIndex].wTimeLow << 16) + pLocalTeams->aStamps[nIndex].wTimeHigh,
                                    pLocalTeams->aStamps[nIndex].wSize,
                                    (pRemoteTeams->aStamps[nIndex].wTimeLow << 16) + pRemoteTeams->aStamps[nIndex].wTimeHigh,
                                    pRemoteTeams->aStamps[nIndex].wSize);

                    return 0;
                }
            }

            if (pLocalArenas->nCount != pRemoteArenas->nCount)
            {
                // STRING: HYPERX 0x004c1cd8
                g_gameApp.Error("%s number of ARENA (*.ENV) files does not match\n", ASSETS_DIFFER);

                return 0;
            }

            for (nIndex = 0; nIndex < pLocalArenas->nCount; nIndex++)
            {
                if (pRemoteArenas->aStamps[nIndex].wTimeLow != pLocalArenas->aStamps[nIndex].wTimeLow || pRemoteArenas->aStamps[nIndex].wTimeHigh != pLocalArenas->aStamps[nIndex].wTimeHigh || pRemoteArenas->aStamps[nIndex].wSize != pLocalArenas->aStamps[nIndex].wSize)
                {
                    // STRING: HYPERX 0x004c1c74
                    g_gameApp.Error("%s file %d:%s does not match\nhave=%X,%X\nneed=%X,%X",
                                    ASSETS_DIFFER,
                                    nIndex,
                                    SceneInfo::ArenaName(pInfo, (short)nIndex),
                                    (pLocalArenas->aStamps[nIndex].wTimeLow << 16) + pLocalArenas->aStamps[nIndex].wTimeHigh,
                                    pLocalArenas->aStamps[nIndex].wSize,
                                    (pRemoteArenas->aStamps[nIndex].wTimeLow << 16) + pRemoteArenas->aStamps[nIndex].wTimeHigh,
                                    pRemoteArenas->aStamps[nIndex].wSize);

                    return 0;
                }
            }

            if (pLocalPlans->nCount != pRemotePlans->nCount)
            {
                // STRING: HYPERX 0x004c1ca8
                g_gameApp.Error("%s number of PLAN (*.PBN) files does not match\n", ASSETS_DIFFER);

                return 0;
            }

            for (nIndex = 0; nIndex < pLocalPlans->nCount; nIndex++)
            {
                if (pRemotePlans->aStamps[nIndex].wTimeLow != pLocalPlans->aStamps[nIndex].wTimeLow || pRemotePlans->aStamps[nIndex].wTimeHigh != pLocalPlans->aStamps[nIndex].wTimeHigh || pRemotePlans->aStamps[nIndex].wSize != pLocalPlans->aStamps[nIndex].wSize)
                {
                    // STRING: HYPERX 0x004c1b18
                    GameSession::GetPlansEntry("PLANS.CAT", szName, nIndex);

                    // STRING: HYPERX 0x004c1c3c
                    g_gameApp.Error("%s file %d:%s.PBN does not match\nhave=%X,%X\nneed=%X,%X",
                                    ASSETS_DIFFER,
                                    nIndex,
                                    szName,
                                    (pLocalPlans->aStamps[nIndex].wTimeLow << 16) + pLocalPlans->aStamps[nIndex].wTimeHigh,
                                    pLocalPlans->aStamps[nIndex].wSize,
                                    (pRemotePlans->aStamps[nIndex].wTimeLow << 16) + pRemotePlans->aStamps[nIndex].wTimeHigh,
                                    pRemotePlans->aStamps[nIndex].wSize);

                    return 0;
                }
            }
        }

        g_pGameEngine->SetDifficultyLevel(pRemoteSettings->nDifficulty);
        g_pGameEngine->SetBrutality(pRemoteSettings->nBrutality);
        g_pGameEngine->SetPeriodLengthMs(pRemoteSettings->nPeriod * PERIOD_SCALE);

        g_nDromeArena    = pRemoteSettings->nArena;
        g_nDromeTeamAway = pRemoteSettings->nTeamTwo;
        g_nDromeTeamHome = pRemoteSettings->nTeamOne;

        return 1;
    }
}

// Frees the catalogue's buffer and wipes both descriptors.
// FUNCTION: HYPERX 0x00442830
int GameSession::FreeAssetBuffer(SyncBlocks* pCatalog, SyncBlocks* pDescriptor)
{
    if (pCatalog->apBlocks[SYNC_BLOCK_SETTINGS] != 0)
    {
        std::free(pCatalog->apBlocks[SYNC_BLOCK_SETTINGS]);
    }

    pCatalog->apBlocks[SYNC_BLOCK_SETTINGS] = 0;
    pCatalog->apBlocks[SYNC_BLOCK_TEAMS]    = 0;
    pCatalog->apBlocks[SYNC_BLOCK_ARENAS]   = 0;
    pCatalog->apBlocks[SYNC_BLOCK_PLANS]    = 0;

    pDescriptor->apBlocks[SYNC_BLOCK_SETTINGS] = 0;
    pDescriptor->apBlocks[SYNC_BLOCK_TEAMS]    = 0;
    pDescriptor->apBlocks[SYNC_BLOCK_ARENAS]   = 0;
    pDescriptor->apBlocks[SYNC_BLOCK_PLANS]    = 0;

    return 1;
}

// FUNCTION: HYPERX 0x004793b0
void GameSession::Disconnect(void)
{
    GameSession* const pSession = g_pGameSession;

    g_bSessionActive = 0;
    g_nSessionState  = 0;

    if (pSession != 0)
    {
        g_pGameSession = pSession->Destroy();

        g_pDPlayLobby = 0;
    }
}

// Both answer nought past the end of the table.
static char* SceneInfo::TeamName(const TeamsBlock* pInfo, short nIndex)
{
    if (nIndex < pInfo->GetTeamCount())
    {
        return pInfo->m_ppTeams[nIndex]->szPath;
    }

    return 0;
}

static char* SceneInfo::ArenaName(const TeamsBlock* pInfo, short nIndex)
{
    if (nIndex < pInfo->GetTeamCount())
    {
        return pInfo->m_ppTeams[nIndex]->szEnvironment;
    }

    return 0;
}
