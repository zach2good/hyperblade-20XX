#include "common/compat.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "engine/objectclasses.h"
#include "engine/modelids.h"
#include "session.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "planvm/planvm.h"
#include <string.h>
#include "cplayer.h"
#include "gamesession.h"
#include "app/gameapp.h"
#include "engine/engineobject.h"
#include "net/netsetup.h"
#include "net/network.h"
#include "net/syncblock.h"
#include "net/netsetupstate.h"
#include "net/netsendflags.h"
#include "engine/objectphase.h"
#include <SDL3/SDL.h>

#define PLAYER_BUFFER_BYTES 58

extern NetSendFlags g_aSendFlagsByType[22];

#define SEND_QUEUE_LIMIT       0xf
#define SEND_QUEUE_SPIN_FLOOR  4
#define SEND_QUEUE_MS_PER_UNIT 100

#define PLAYER_PACKET_BYTES 58

// The buffers and the state block are inside the object; pState at +0x270 points at the block.

// FUNCTION: HYPERX 0x0042d1f0
CPlayer::CPlayer(GameSession* pSession)
{
    InitDirectPlayParams();

    pBufferB     = &m_sendBuf;
    pSendPayload = GetSendPayloadPtr();
    pBufferA     = &m_recvBuf;
    pState       = &stateBlock;

    memset(pBufferB, 0, PLAYER_BUFFER_BYTES);
    memset(pBufferA, 0, PLAYER_BUFFER_BYTES);

    pDPlay       = pSession->pDPlay;
    hPacketEvent = 0;

    g_hNetworkThread = 0;

    nThreadId = 0;
    nUnusedC  = 1;

    pState->nPlayerId           = DPID_UNKNOWN;
    pState->nConnected          = 0;
    pState->nHuman              = 0;
    pState->nObjectSlot         = MODEL_ID_NONE;
    pState->slotName.cSlotFlags = 0;
    pState->bHost               = 0;
    pState->nSlotFlags          = 0;
    pState->nReady              = 0;
    pState->nActive             = 0;
    pState->bLocal              = 0;
    pState->nDisconnected       = 0;

    nSendFlags  = 0;
    nRecvFlags  = DPRECEIVE_TOPLAYER;
    nQueueCount = 0;
    nQueueHead  = 0;

    InitializeCriticalSection(&m_lock);
    InitializeCriticalSection(&m_dplayLock);
}

// Drains what DirectPlay holds for the player, stops its receive thread and destroys it.

// FUNCTION: HYPERX 0x0042d370
CPlayer::~CPlayer()
{
    if (g_hNetworkThread != 0 && pDPlay != 0)
    {
        DWORD nPending = 0;

        // Nought is DP_OK; nPending is never read, so the drain runs with nothing pending too.
        if (pDPlay->GetMessageCount(pState->nPlayerId, &nPending) == 0)
        {
            while (DispatchPacket() == 0)
            {
            }
        }

        StopNetworkThread();
    }

    PlayerState* pInfo = pState;

    if (pInfo->nSlotFlags != 0 && pInfo->nConnected == 0 && pInfo->bLocal != 0 && pInfo->nPlayerId != DPID_UNKNOWN)
    {
        const int nResult = pDPlay->DestroyPlayer(pInfo->nPlayerId);

        if (nResult != 0)
        {
            // STRING: HYPERX 0x004bcd78
            g_gameApp.Error("pIDC->DestroyPlayer failed with %s\n",
                            DirectPlay::GetErrorString(nResult));
        }
    }

    DetachObject();
}

// Both buffers are 58 bytes, and a null one is skipped.

// FUNCTION: HYPERX 0x0042d460
void CPlayer::ResetState()
{
    pBufferB->message.nSlot = (unsigned char)pState->nSlotFlags;

    EnterCriticalSection(&m_lock);

    nQueueCount = 0;
    nQueueHead  = 0;

    if (pBufferB != 0)
    {
        memset(pBufferB, 0, PLAYER_PACKET_BYTES);
    }

    if (pBufferA != 0)
    {
        memset(pBufferA, 0, PLAYER_PACKET_BYTES);
    }

    memset(&vLastVelocity, 0, sizeof(vLastVelocity));
    memset(&vLastPosition, 0, sizeof(vLastPosition));

    cLastBehaviour = 0;
    cLastPlan      = 0;

    LeaveCriticalSection(&m_lock);
}

// The second drain loop runs only for the player that drives God.
// FUNCTION: HYPERX 0x0042d4f0
void CPlayer::AssignObject(int nObject)
{
    int nPayload;
    int nSecond;

    if (nObject != MODEL_ID_NONE)
    {
        pObject = g_pGameEngine->m_ppSlots[(short)nObject];

        if (pObject != 0)
        {
            pBody             = pObject->GetPhysicsBody();
            pObject->m_nPhase = OBJECT_PHASE_LOCAL;

            while (pObject->DequeueEvent(&nPayload, &nSecond) != 0)
            {
            }

            if (pState->nObjectSlot == SLOT_GOD)
            {
                while (pObject->PollStationaryEvents(&nPayload, &nSecond) != 0)
                {
                }
            }
        }
        else
        {
            pBody = 0;
        }
    }
    else
    {
        pObject = 0;
        pBody   = 0;
    }

    if (pBody != 0)
    {
        pBody->pPlayer = this;
    }

    if (pState->nObjectSlot == SLOT_GOD)
    {
        for (int nIndex = 0; nIndex < g_nStationaryCount; nIndex++)
        {
            EngineObject* const pStationary = g_pGameEngine->m_ppSlots[(short)g_aStationarySlots[nIndex]];

            pStationary->GetPhysicsBody()->pPlayer = this;
            pStationary->m_nPhase                  = OBJECT_PHASE_LOCAL;
        }
    }
}

// GetLastError is called and its answer dropped.
// FUNCTION: HYPERX 0x0042d610
void CPlayer::DetachObject()
{
    int nPayload;
    int nSecond;

    if (g_hNetworkThread != 0 && CPlayer::StopNetworkThread() == 0)
    {
        GetLastError();
    }

    if (pObject != 0)
    {
        pObject->m_nPhase = OBJECT_PHASE_LOCAL;

        while (pObject->DequeueEvent(&nPayload, &nSecond) != 0)
        {
        }

        if (pState->nObjectSlot == SLOT_GOD)
        {
            while (pObject->PollStationaryEvents(&nPayload, &nSecond) != 0)
            {
            }
        }

        pObject = 0;
    }

    if (pBody != 0)
    {
        pBody->pPlayer = 0;
        pBody          = 0;
    }

    if (pState->nObjectSlot == SLOT_GOD)
    {
        for (int nIndex = 0; nIndex < g_nStationaryCount; nIndex++)
        {
            EngineObject* const pStationary        = g_pGameEngine->m_ppSlots[(short)g_aStationarySlots[nIndex]];
            pStationary->GetPhysicsBody()->pPlayer = 0;
            pStationary->m_nPhase                  = OBJECT_PHASE_LOCAL;
        }
    }

    pState->nPlayerId           = DPID_UNKNOWN;
    pState->nConnected          = 0;
    pState->nHuman              = 0;
    pState->nObjectSlot         = MODEL_ID_NONE;
    pState->slotName.cSlotFlags = 0;
    pState->bHost               = 0;
    pState->nSlotFlags          = 0;
    pState->nActive             = 0;
    pState->nReady              = 0;
    pState->bLocal              = 0;
    pState->nDisconnected       = 0;

    EnterCriticalSection(&m_lock);

    nQueueCount = 0;
    nQueueHead  = 0;

    if (pBufferB != 0)
    {
        memset(pBufferB, 0, PLAYER_BUFFER_BYTES);
    }

    if (pBufferA != 0)
    {
        memset(pBufferA, 0, PLAYER_BUFFER_BYTES);
    }

    memset(&vLastVelocity, 0, sizeof(vLastVelocity));
    memset(&vLastPosition, 0, sizeof(vLastPosition));

    cLastBehaviour = 0;
    cLastPlan      = 0;

    LeaveCriticalSection(&m_lock);
}

#define PACKET_KIND_DATA 0x0c
#define PACKET_OVERHEAD  3

#define QUEUE_LIMIT 0x96

// FUNCTION: HYPERX 0x0042d810
char CPlayer::EnqueueNetRecord(const NetMoveRecord* pRecord)
{
    while (nQueueCount + 1 > QUEUE_LIMIT)
    {
        nQueueHead = (nQueueHead + 1) % QUEUE_LIMIT;
        nQueueCount--;
    }

    aQueue[(nQueueHead + nQueueCount) % QUEUE_LIMIT] = *pRecord;

    nQueueCount++;

    return 1;
}

// FUNCTION: HYPERX 0x0042d8a0
NetMoveRecord* CPlayer::DequeueNetRecord()
{
    const int nCount = nQueueCount;

    if (nCount == 0)
    {
        return 0;
    }

    NetMoveRecord* const pRecord = &aQueue[nQueueHead];
    nQueueHead                   = (nQueueHead + 1) % QUEUE_LIMIT;
    nQueueCount                  = nCount - 1;

    return pRecord;
}

// A type not in the table keeps the player's default flag; a queue depth throttles the send.

// FUNCTION: HYPERX 0x0042d8f0
int __fastcall CPlayer::SendPacket(DWORD idTo, NetMessage* pMessage, DWORD dwDataSize)
{
    IDirectPlay* pDirectPlay;

    int                nResult = E_FAIL;
    const unsigned int nType   = pMessage->nKind;
    int                nFlags  = nSendFlags;
    CPlayer* const     pLocal  = g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER];

    if (pLocal == 0 || g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->pState->nPlayerId == DPID_UNKNOWN)
    {
        // STRING: HYPERX 0x004bcd9c
        g_gameApp.Error("Attempting to do a Send() without a MSDP player\n");
    }
    else
    {
        for (int i = 0; i < 22; i++)
        {
            if ((unsigned int)g_aSendFlagsByType[i].nKind == nType)
            {
                nFlags = g_aSendFlagsByType[i].nFlags;

                break;
            }
        }

        pDirectPlay = this->pDPlay;
        nResult     = pDirectPlay->Send(pLocal->pState->nPlayerId, idTo, nFlags, pMessage, dwDataSize);

        if (nResult > 0 && nResult < SEND_QUEUE_LIMIT)
        {
            const DWORD dwDeadline = (DWORD)SDL_GetTicks() + nResult * SEND_QUEUE_MS_PER_UNIT;

            if (nResult > SEND_QUEUE_SPIN_FLOOR)
            {
                while (dwDeadline > (DWORD)SDL_GetTicks())
                {
                }
            }
        }
    }

    return nResult;
}

// One global handle serves every player; the thread runs time-critical.

// FUNCTION: HYPERX 0x0042d9c0
HANDLE CPlayer::StartNetworkThread(HANDLE hEvent)
{
    if (hEvent != 0)
    {
        hPacketEvent = hEvent;

        g_hNetworkThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CPlayer::NetworkReceiveThreadProc, this, 0, &nThreadId);

        if (g_hNetworkThread != 0)
        {
            SetThreadPriority(g_hNetworkThread, THREAD_PRIORITY_TIME_CRITICAL);

            return g_hNetworkThread;
        }

        // STRING: HYPERX 0x004bcdd0
        g_gameApp.Error("CreateThread failed for %s! LastError Code %0X\n",
                        pState->slotName.GetWireName(),
                        GetLastError());
    }

    return g_hNetworkThread;
}

// The global handle is also the shutdown flag, so it is closed before the event is set.

// FUNCTION: HYPERX 0x0042da40
int CPlayer::StopNetworkThread()
{
    if (g_hNetworkThread != 0)
    {
        CloseHandle(g_hNetworkThread);

        g_hNetworkThread = 0;

        if (hPacketEvent != 0)
        {
            SetEvent(hPacketEvent);
        }
    }

    if (hPacketEvent != 0)
    {
        CloseHandle(hPacketEvent);
    }

    g_hNetworkThread = 0;

    hPacketEvent = 0;
    nThreadId    = 0;

    return 1;
}

// Once the loop is entered the only way out is ExitThread.

// FUNCTION: HYPERX 0x0042daa0
unsigned int CPlayer::NetworkReceiveLoop()
{
    if (g_hNetworkThread != 0)
    {
        while (g_hNetworkThread != 0)
        {
            if (WaitForSingleObject(hPacketEvent, INFINITE) == WAIT_OBJECT_0)
            {
                while (g_hNetworkThread != 0 && DispatchPacket() == 0)
                {
                }

                if (g_hNetworkThread == 0)
                {
                    break;
                }

                ResetEvent(hPacketEvent);
            }
        }

        ExitThread(0);
    }

    return 0;
}

#define RECV_BUFFER_BYTES 0x100

#define MSG_ASSET_REQUEST    1
#define MSG_PLAYER_CONNECTED 8
#define MSG_SLOT_ACCEPTED    9
#define MSG_SLOT_REJECTED    10
#define MSG_ADD_PLAYER       0x0b
#define MSG_CHAT             0x0c
#define MSG_PLAYER_READY     0x0d
#define MSG_HOST_START       0x0e
#define MSG_MOVE_FIRST       0x0f
#define MSG_MOVE_LAST        0x12
#define MSG_STATIC_FIRST     0x13
#define MSG_STATIC_LAST      0x14
#define MSG_DROP_PLAYER      0x15

// Messages from this player's own id are dropped; group, session and connect notices are ignored.
// FUNCTION: HYPERX 0x0042db10
unsigned int __fastcall CPlayer::DispatchPacket()
{
    IDirectPlay* pDirectPlay;
    PlayerState* pPlayerState;
    PlayerBase*  pMoveTarget;
    PlayerBase*  pStaticTarget;
    unsigned int nSlot;

    DWORD idFrom = DPID_UNKNOWN;
    DWORD idTo   = this->pState->nPlayerId;
    DWORD dwSize = RECV_BUFFER_BYTES;

    if (this != g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER])
    {
        // STRING: HYPERX 0x004bced8
        g_gameApp.Error("RCV: Received by player %d which is not the MSDP player",
                        this->pState->nSlotFlags);
    }

    CRITICAL_SECTION* const pLock = &m_dplayLock;

    EnterCriticalSection(pLock);

    pDirectPlay                 = this->pDPlay;
    const unsigned int hr       = pDirectPlay->Receive(&idFrom, &idTo, nRecvFlags, pBufferA, &dwSize);
    const unsigned int nMessage = pBufferA->system.dwType;
    nSlot                       = pBufferA->message.nSlot;

    LeaveCriticalSection(pLock);

    if (hr != DPERR_NOMESSAGES)
    {
        if (hr != 0)
        {
            // STRING: HYPERX 0x004bceb4
            g_gameApp.Error("CPlayer::Receive failed with %s\n",
                            DirectPlay::GetErrorString(hr));
        }
        else
        {
            pPlayerState = this->pState;

            if (idFrom == pPlayerState->nPlayerId)
            {
                return 0;
            }

            if (idFrom == DPID_SYSMSG)
            {
                switch (nMessage)
                {
                    case DPSYS_DELETEPLAYER:
                        EnterCriticalSection(pLock);
                        Network::RemoveDPID(g_pGameSession, pBufferA->deletePlayer.dpId);
                        LeaveCriticalSection(pLock);

                        break;

                    case DPSYS_ADDPLAYER:
                        EnterCriticalSection(pLock);
                        Network::AddDPID(g_pGameSession, pBufferA->addPlayer.dpId);
                        LeaveCriticalSection(pLock);

                        break;

                    case DPSYS_ADDPLAYERTOGROUP:
                        break;

                    case DPSYS_DELETEGROUP:
                        break;

                    case DPSYS_DELETEPLAYERFROMGRP:
                        break;

                    case DPSYS_SESSIONLOST:
                        break;

                    case DPSYS_CONNECT:
                        break;

                    default:
                        // STRING: HYPERX 0x004bce80
                        g_gameApp.Error("CPlayer 0x%X received Unknown System Message %X!\n",
                                        m_nSlot,
                                        nMessage);
                        break;
                }
            }
            else
            {
                switch (nMessage & 0xff)
                {
                    case MSG_ASSET_REQUEST:
                        Network::SendAssetData(idFrom);

                        break;

                    case ASSET_TYPE_SETTINGS:
                    case ASSET_TYPE_TEAMS:
                    case ASSET_TYPE_ARENAS:
                    case ASSET_TYPE_PLANS:
                        EnterCriticalSection(pLock);
                        Network::ReceiveAssetPacket(&pBufferA->asset, dwSize, idFrom);
                        LeaveCriticalSection(pLock);

                        break;

                    case MSG_CHAT:
                        EnterCriticalSection(pLock);
                        NetSetup::DisplayChatMessage(
                            idFrom, pBufferA->message.szPayload, pBufferA->message.nSlot);
                        LeaveCriticalSection(pLock);

                        break;

                    case MSG_PLAYER_CONNECTED:
                        Network::OnPlayerConnected(idFrom, nSlot);

                        break;

                    case MSG_SLOT_ACCEPTED:
                        Network::OnAcceptPlayerSlot(nSlot);

                        break;

                    case MSG_SLOT_REJECTED:
                        Network::OnRejectPlayerSlot(nSlot);

                        break;

                    case MSG_ADD_PLAYER:
                        EnterCriticalSection(pLock);
                        NetSetup::ProcessAddPlayerMsg(idFrom, nSlot, &pBufferA->playerName.name);
                        LeaveCriticalSection(pLock);

                        break;

                    case MSG_PLAYER_READY:
                        NetSetup::OnPlayerReady(nSlot);

                        break;

                    case MSG_HOST_START:
                        NetSetup::OnHostStartGame();

                        break;

                    case MSG_DROP_PLAYER:
                        EnterCriticalSection(pLock);
                        Network::RemoveDPID(g_pGameSession, idFrom);
                        LeaveCriticalSection(pLock);

                        break;

                    case MSG_MOVE_FIRST:
                    case 0x10:
                    case 0x11:
                    case MSG_MOVE_LAST:
                        pMoveTarget = g_pGameSession->apPlayers[nSlot];

                        if (pMoveTarget != 0)
                        {
                            EnterCriticalSection(pLock);
                            pMoveTarget->RecvMovementPacket(pBufferA,
                                                            dwSize);
                            LeaveCriticalSection(pLock);
                        }
                        else
                        {
                            // STRING: HYPERX 0x004bce54
                            g_gameApp.Error("RCV: MovingPacket for unknown player %X!\n", nSlot);
                        }

                        break;

                    case MSG_STATIC_FIRST:
                    case MSG_STATIC_LAST:
                        pStaticTarget = g_pGameSession->apPlayers[NET_SLOT_GOD];

                        if (pStaticTarget != 0)
                        {
                            EnterCriticalSection(pLock);
                            pStaticTarget->RecvStaticObjectPacket(pBufferA,
                                                                  dwSize);
                            LeaveCriticalSection(pLock);
                        }
                        else
                        {
                            // STRING: HYPERX 0x004bce30
                            g_gameApp.Error("RCV: STATIC packet for NULL REF\n");
                        }

                        break;

                    default:
                        // STRING: HYPERX 0x004bce00
                        g_gameApp.Error("CPlayer %s received Unknown Message Type%X!\n",
                                        pPlayerState->slotName.GetWireName(),
                                        nMessage & 0xff);
                        break;
                }
            }
        }
    }

    return hr;
}

// The DirectPlay destination id rides in EDX; every caller passes DPID_ALLPLAYERS.

// FUNCTION: HYPERX 0x0042df20
void __fastcall CPlayer::SendHostStart(unsigned long idTo)
{
    NetShortMessage msg;

    msg.nKind  = MSG_HOST_START;
    msg.nValue = (unsigned char)pState->nSlotFlags;

    SendPacket(idTo, &msg, sizeof(msg));
}

// FUNCTION: HYPERX 0x0042df50
void __fastcall CPlayer::SendPlayerReady(unsigned long idTo)
{
    NetShortMessage msg;

    msg.nKind  = MSG_PLAYER_READY;
    msg.nValue = (unsigned char)pState->nSlotFlags;

    SendPacket(idTo, &msg, sizeof(msg));
}

// FUNCTION: HYPERX 0x0042df80
void __fastcall CPlayer::SendPlayerConnected(unsigned long idTo, int nValue)
{
    NetShortMessage msg;

    msg.nKind  = MSG_PLAYER_CONNECTED;
    msg.nValue = (unsigned char)nValue;

    SendPacket(idTo, &msg, sizeof(msg));
}

// FUNCTION: HYPERX 0x0042dfa0
void __fastcall CPlayer::SendSlotAccepted(unsigned long idTo, char cValue)
{
    NetShortMessage msg;

    msg.nKind  = MSG_SLOT_ACCEPTED;
    msg.nValue = cValue;

    SendPacket(idTo, &msg, sizeof(msg));
}

// FUNCTION: HYPERX 0x0042dfc0
void __fastcall CPlayer::SendSlotRejected(unsigned long idTo, char cValue)
{
    NetShortMessage msg;

    msg.nKind  = MSG_SLOT_REJECTED;
    msg.nValue = cValue;

    SendPacket(idTo, &msg, sizeof(msg));
}

// The text is a wire name, so the copy fills the name record and the whole message goes.

// FUNCTION: HYPERX 0x0042dfe0
void __fastcall CPlayer::SendStringMsg(unsigned long idTo, int nValue, const char* pszText)
{
    NetPlayerNameMessage msg;

    msg.nSlot = (unsigned char)nValue;
    msg.nKind = MSG_ADD_PLAYER;

    strcpy(msg.name.GetWireName(), pszText);

    SendPacket(idTo, &msg, sizeof(msg));
}

// FUNCTION: HYPERX 0x0042e040
void __fastcall CPlayer::SendDataMsg(unsigned long idTo, const void* pData, int nLength)
{
    NetSendBuffer* const  pSend   = pBufferB;
    NetDataMessage* const pPacket = &pSend->data;

    if (nLength > NET_DATA_MAX)
    {
        nLength = NET_DATA_MAX;
    }

    pPacket->nKind   = PACKET_KIND_DATA;
    pPacket->nLength = (unsigned char)nLength;

    // One more byte than the length: the payload carries its own terminator.
    memcpy(pPacket->abData, pData, nLength + 1);

    SendPacket(idTo, pPacket, nLength + PACKET_OVERHEAD);
}

// FUNCTION: HYPERX 0x0042e0a0
void __fastcall CPlayer::SendRawMsg(unsigned long idTo, NetMessage* pMessage, int nLength)
{
    SendPacket(idTo, pMessage, nLength);
}

// Sends one byte and leaves the second as it found it.

// FUNCTION: HYPERX 0x0042e0c0
void __fastcall CPlayer::SendAssetRequest(unsigned long idTo)
{
    NetShortMessage msg;

    msg.nKind = MSG_ASSET_REQUEST;

    SendPacket(idTo, &msg, sizeof(msg));
}

// FUNCTION: HYPERX 0x0042e0e0
int __fastcall CPlayer::SendDropPlayer(unsigned long idTo)
{
    NetShortMessage msg;

    msg.nKind = MSG_DROP_PLAYER;

    if (g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER] != 0 && g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->pState->nPlayerId != DPID_UNKNOWN)
    {
        return SendPacket(idTo, &msg, sizeof(msg));
    }

    return 0;
}

// FUNCTION: HYPERX 0x0042e120
unsigned int __cdecl CPlayer::NetworkReceiveThreadProc(CPlayer* pPlayer)
{
    return pPlayer->NetworkReceiveLoop();
}

// Last in the file so the unit's literals follow it. The walk is bounded by the count, and entry zero matches kind zero before the trailing one can.
// GLOBAL: HYPERX 0x004bccc8
NetSendFlags g_aSendFlagsByType[22] = {
    { 0x00, 0 }, { 0x01, DPSEND_GUARANTEED }, { 0x02, DPSEND_GUARANTEED }, { 0x03, DPSEND_GUARANTEED }, { 0x04, DPSEND_GUARANTEED }, { 0x05, DPSEND_GUARANTEED }, { 0x06, 0 }, { 0x07, 0 }, { 0x08, DPSEND_GUARANTEED }, { 0x09, DPSEND_GUARANTEED }, { 0x0a, DPSEND_GUARANTEED }, { 0x0b, DPSEND_GUARANTEED }, { 0x0c, DPSEND_GUARANTEED }, { 0x0d, DPSEND_GUARANTEED }, { 0x0e, DPSEND_GUARANTEED }, { 0x0f, 0 }, { 0x10, 0 }, { 0x11, 0 }, { 0x12, 0 }, { 0x13, 0 }, { 0x14, 0 }, { 0x00, 0 }
};
