#include "common/compat.h"
#include "physics/physicsbody.h"
#include "session.h"
#include "engine/devicector.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "globals.h"
#include "netglobals.h"
#include "netrecords.h"
#include "math/vector.h"
#include "playerbase.h"
#include "app/gameapp.h"
#include "gamesession.h"
#include "cplayer.h"
#include <string.h>
#include "engine/engineobject.h"
#include "net/movecodec.h"
#include "net/netsequenceword.h"
#include "net/slotnibbles.h"

#define PACKET_TYPE 0x13

#define EVENT_SHORT offsetof(StationaryEvent, command.nPayload)

#define PACKET_LIMIT 0xf1

// FUNCTION: HYPERX 0x0042be90
PlayerBase::~PlayerBase()
{
}

// Three of the fields come from the object's own vtable rather than a constant.

// FUNCTION: HYPERX 0x0042bea0
void PlayerBase::InitDirectPlayParams()
{
    pObject = 0;
    pBody   = 0;

    pBufferB     = GetSendBufBase();
    pSendPayload = GetSendPayloadPtr();
    pBufferA     = GetRecvBufBase();

    memset(&m_vPrevVel, 0, sizeof(m_vPrevVel));

    memset(&m_vPrevPos, 0, sizeof(m_vPrevPos));

    m_nPrevPlan      = 0;
    m_nPrevBehaviour = 0;
    m_nLastSend      = 0;
}

// How long a slot waits before it sends again.
#define SEND_INTERVAL_MS 200
#define FULL_MS          1000

#define PACKET_ENCODED 0x10
#define PACKET_RAW     0x0f
#define PACKET_EVENTS  0x12

#define LENGTH_ENCODED 0xb
#define LENGTH_RAW     0x1a

#define EVENT_LIMIT 0xf2

#define EVENT_HAS_PARAM 0x40000000

#define CARRIER_NONE 0xff

// A rok packet carries the carrier's slot, 0xff if none; the packet goes to every remote id.
// FUNCTION: HYPERX 0x0042c300
void __fastcall PlayerBase::BroadcastState(NetSendBuffer* pPacket, int nLength, int nObjectSlot)
{
    AttachPak* pCarrier;
    short      nCarrier;

    if (nObjectSlot == SLOT_ROK)
    {
        pCarrier = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        if (pCarrier != 0 && (nCarrier = pCarrier->nSlot) < PLAYER_OBJECT_COUNT)
        {
            pPacket->aBytes[nLength] = (unsigned char)nCarrier;
            nLength++;
        }
        else
        {
            pPacket->aBytes[nLength] = CARRIER_NONE;
            nLength++;
        }
    }

    for (int i = 0; i < g_pGameSession->nRemoteCount; i++)
    {
        SendPacket(g_pGameSession->aRemoteIds[i], &pPacket->message, nLength);
    }
}

#define EVENT_NONE 0xffffffff

// A move goes out every second regardless; between those only when it changed, the Rok is not held and a modem link is not throttled.
// FUNCTION: HYPERX 0x0042c380
void PlayerBase::NetUpdateObject(unsigned char nSlot)
{
    PhysicsBody* pRokBody;
    Vec3         vStep;
    Vec3         vPos;
    int          nSecond;
    unsigned int nUsed;
    DWORD        dwNow;

    int nPayload = EVENT_NONE;

    vPos  = pBody->pos;
    vStep = pBody->step;

    const unsigned char nObjectSlot = (unsigned char)pObject->GetSlot();

    int bHeld      = 0;
    int bThrottled = 0;

    const unsigned char nPlan      = (unsigned char)pObject->m_wSyncedPlan;
    const unsigned char nBehaviour = (unsigned char)pObject->m_wSyncedBehaviour;

    if (nObjectSlot == SLOT_ROK)
    {
        pRokBody = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody();

        if (pRokBody->pAttach != 0 && pRokBody->pAttach->nSlot < PLAYER_OBJECT_COUNT)
        {
            bHeld = 1;
        }
    }

    if (g_bModemPlay != 0)
    {
        dwNow = timeGetTime();

        if (m_nLastSend + (unsigned int)SEND_INTERVAL_MS > dwNow)
        {
            bThrottled = 1;
        }
    }

    EnterCriticalSection(&m_dplayLock);

    dwNow = timeGetTime();

    if (m_nLastSend + (unsigned int)FULL_MS < dwNow || (bThrottled == 0 && bHeld == 0 && (memcmp(&m_vPrevVel, &vStep, sizeof(Vec3)) != 0 || memcmp(&m_vPrevPos, &vPos, sizeof(Vec3)) != 0)))
    {
        MovePacket* const pMove = &pBufferB->move;

        nUsed = LENGTH_ENCODED;

        pMove->nKind = PACKET_ENCODED;
        pMove->nSlot = nSlot;

        if (g_bMovePackingEnabled == 0 || MoveCodec::Encode(&pMove->packed, &vPos, &vStep) == 0)
        {
            pMove->nKind = PACKET_RAW;

            nUsed = LENGTH_RAW;

            pMove->raw.vPos  = vPos;
            pMove->raw.vStep = vStep;
        }

        BroadcastState(pBufferB, nUsed, nObjectSlot);

        m_nLastSend = timeGetTime();
    }

    if (pObject->DequeueEvent(&nPayload, &nSecond) != 0)
    {
        EventsPacket* const pEvents = &pBufferB->events;

        nUsed = 5;

        pEvents->nKind      = PACKET_EVENTS;
        pEvents->nSlot      = nSlot;
        pEvents->nPlan      = nPlan;
        pEvents->nBehaviour = nBehaviour;
        pEvents->nCount     = 0;

        while (nPayload != EVENT_NONE)
        {
            pEvents->anWords[pEvents->nCount++] = nPayload;

            if (nSecond != 0 || g_bMovePackingEnabled == 0)
            {
                pEvents->anWords[pEvents->nCount - 1] |= EVENT_HAS_PARAM;
                pEvents->anWords[pEvents->nCount++] = nSecond;
            }

            nUsed = pEvents->nCount * 4 + 5;

            if (nUsed > EVENT_LIMIT)
            {
                BroadcastState(pBufferB, nUsed, nObjectSlot);

                pEvents->nCount = 0;
                nUsed           = 0;
            }

            pObject->DequeueEvent(&nPayload, &nSecond);
        }

        if (nUsed != 0)
        {
            BroadcastState(pBufferB, nUsed, nObjectSlot);
        }
    }

    LeaveCriticalSection(&m_dplayLock);

    m_vPrevVel       = vStep;
    m_vPrevPos       = vPos;
    m_nPrevPlan      = nPlan;
    m_nPrevBehaviour = nBehaviour;
}

// Two stationary slots per nibble byte, then five-byte events, nine with a parameter; flushed at 0xf1 bytes.

// FUNCTION: HYPERX 0x0042c670
void PlayerBase::NetUpdateMultiObject(int)
{
    int           nPayload;
    int           nSecond;
    unsigned char nCode;
    unsigned int  nUsed;
    short         nSlot;

    const unsigned char nObjectSlot = (unsigned char)pObject->GetSlot();

    EnterCriticalSection(&m_dplayLock);

    nCode = pObject->PollStationaryEvents(&nPayload, &nSecond);

    if (nCode != 0)
    {
        NetSendBuffer* const pPacket = pBufferB;

        StationaryPacket* const pHeader = &pPacket->stationary;

        pHeader->nKind       = PACKET_TYPE;
        pHeader->nEventBytes = 0;

        const unsigned int nRemaining = (g_nStationaryCount + 1) / 2;

        memset(pHeader->abSlotNibbles, 0, nRemaining);

        // Two slots to a byte, the even slot in the low nibble.
        for (int nIndex = 0; nIndex < g_nStationaryCount; nIndex++)
        {
            nSlot = (short)g_aStationarySlots[nIndex];

            pHeader->abSlotNibbles[nIndex / 2] |=
                SlotNibbles::Pack(nIndex, g_pGameEngine->m_ppSlots[nSlot]->m_wSyncedBehaviour);
        }

        unsigned char* pAt = pHeader->abSlotNibbles + (g_nStationaryCount + 1) / 2;

        do
        {
            StationaryEvent* const   pEvent   = (StationaryEvent*)pAt;
            StationaryCommand* const pCommand = &pEvent->command;

            pEvent->nSlot      = (unsigned char)g_aStationarySlots[nCode - 1];
            pCommand->nCommand = nPayload;

            pHeader->nEventBytes += EVENT_SHORT;

            pAt += EVENT_SHORT;

            if (nSecond != 0 || g_bMovePackingEnabled == 0)
            {
                pCommand->nCommand |= EVENT_HAS_PARAM;
                pCommand->nPayload = nSecond;

                pHeader->nEventBytes += sizeof(pCommand->nPayload);
                pAt += sizeof(pCommand->nPayload);
            }

            nUsed = (g_nStationaryCount + 1) / 2 + offsetof(StationaryPacket, abSlotNibbles) + pHeader->nEventBytes;

            if (nUsed > PACKET_LIMIT)
            {
                BroadcastState(pPacket, nUsed, nObjectSlot);

                pHeader->nEventBytes = 0;
                nUsed                = 0;

                pAt = pHeader->abSlotNibbles + (g_nStationaryCount + 1) / 2;
            }

            nCode = pObject->PollStationaryEvents(&nPayload, &nSecond);
        } while (nCode != 0);

        if (nUsed != 0)
        {
            BroadcastState(pPacket, nUsed, nObjectSlot);
        }
    }

    LeaveCriticalSection(&m_dplayLock);
}

#define KIND_CODED  0x14
#define NO_SEQUENCE 0xff

// One record is reused, so each event inherits the fields it does not set.
// FUNCTION: HYPERX 0x0042c830
void PlayerBase::RecvStaticObjectPacket(const NetReceiveBuffer* pPacket, int nLength)
{
    const StationaryPacket* const pHeader = &pPacket->stationary;
    NetMoveRecord                 record;

    CPlayer* const pGodPlayer = g_pGameSession->apPlayers[NET_SLOT_GOD];

    record.body.nPlan      = 0;
    record.body.nBehaviour = 0;

    record.body.nCommand       = EVENT_NONE;
    record.body.nPayload       = EVENT_NONE;
    record.body.nSecondCommand = EVENT_NONE;
    record.body.nSecondPayload = EVENT_NONE;

    const int nBitmap = (g_nStationaryCount + 1) / 2;

    record.cKind          = 0;
    record.nSlot          = NET_SLOT_GOD;
    record.body.nSequence = SLOT_GOD;

    const int nExpected = pHeader->nEventBytes + nBitmap + offsetof(StationaryPacket, abSlotNibbles);

    if (nLength != nExpected)
    {
        // STRING: HYPERX 0x004bcc14
        g_gameApp.Error("STATIC_EVENT packet length %d instead of %d\n", nLength, nExpected);

        return;
    }

    memcpy(record.body.abNibbles, pHeader->abSlotNibbles, nBitmap);

    if (pHeader->nKind == KIND_CODED)
    {
        record.body.nPlan = NO_SEQUENCE;
    }

    int                  nDone = 0;
    const unsigned char* pAt   = pHeader->abSlotNibbles + nBitmap;

    while (nDone < pHeader->nEventBytes)
    {
        const StationaryEvent* const pEvent = (const StationaryEvent*)pAt;

        record.body.nSequence = NetSequenceWord::PackTarget(pEvent->nSlot) + SLOT_GOD;
        record.body.nCommand  = pEvent->command.nCommand & ~EVENT_HAS_PARAM;

        if ((pEvent->command.nCommand & EVENT_HAS_PARAM) != 0)
        {
            record.body.nPayload = pEvent->command.nPayload;

            pAt += sizeof(pEvent->command.nPayload);
            nDone += sizeof(pEvent->command.nPayload);
        }
        else
        {
            record.body.nPayload = 0;
        }

        pAt += EVENT_SHORT;

        EnterCriticalSection(&m_lock);
        pGodPlayer->EnqueueNetRecord(&record);
        LeaveCriticalSection(&m_lock);

        nDone += EVENT_SHORT;

        record.body.nPlan = NO_SEQUENCE;
    }
}

#define PACKET_MOVE         0x0f
#define PACKET_PACKED_MOVE  0x10
#define PACKET_MOVING_EVENT 0x12

// The Rok's packets end with its carrier's slot, CARRIER_NONE when nobody holds it.
// FUNCTION: HYPERX 0x0042c970
void PlayerBase::RecvMovementPacket(const NetReceiveBuffer* pPacket, int nLength)
{
    NetMoveRecord             record;
    CPlayer*                  pPlayer;
    LPCRITICAL_SECTION        pLock;
    const MovePacket* const   pMove   = &pPacket->move;
    const EventsPacket* const pEvents = &pPacket->events;

    record.cKind               = 0;
    record.dwTime              = timeGetTime();
    record.body.nCommand       = EVENT_NONE;
    record.body.nPayload       = EVENT_NONE;
    record.body.nSecondCommand = EVENT_NONE;
    record.body.nSecondPayload = EVENT_NONE;
    int nCarrierWord           = 0;

    if (pMove->nSlot == NET_SLOT_ROK)
    {
        nCarrierWord = (unsigned char)pPacket->aBytes[nLength - 1];

        if (nCarrierWord != CARRIER_NONE)
        {
            nCarrierWord = NetSequenceWord::PackCarrier(nCarrierWord);
        }
        else
        {
            nCarrierWord = 0;
        }

        nLength--;
    }

    if (pMove->nKind == PACKET_PACKED_MOVE)
    {
        if (nLength != LENGTH_ENCODED)
        {
            // STRING: HYPERX 0x004bcc9c
            g_gameApp.Error("PACKEDMOVE packet length %d instead of %d\n", nLength, LENGTH_ENCODED);

            return;
        }

        pPlayer                = g_pGameSession->apPlayers[pMove->nSlot];
        record.nSlot           = pMove->nSlot;
        record.body.nSequence  = pPlayer->pState->nObjectSlot + nCarrierWord;
        record.body.nPlan      = NO_SEQUENCE;
        record.body.nBehaviour = NO_SEQUENCE;

        MoveCodec::Decode(&pMove->packed, &record.body.vPosition, &record.body.vVelocity);

        pPlayer->vLastPosition = record.body.vPosition;
        pPlayer->vLastVelocity = record.body.vVelocity;

        pLock = &m_lock;

        EnterCriticalSection(pLock);
        pPlayer->EnqueueNetRecord(&record);
        LeaveCriticalSection(pLock);

        return;
    }

    if (pMove->nKind == PACKET_MOVE)
    {
        if (nLength != LENGTH_RAW)
        {
            // STRING: HYPERX 0x004bcc74
            g_gameApp.Error("MOVE packet length %d instead of %d\n", nLength, LENGTH_RAW);

            return;
        }

        pPlayer                = g_pGameSession->apPlayers[pMove->nSlot];
        record.nSlot           = pMove->nSlot;
        record.body.nSequence  = pPlayer->pState->nObjectSlot + nCarrierWord;
        record.body.nPlan      = NO_SEQUENCE;
        record.body.nBehaviour = NO_SEQUENCE;
        record.body.vPosition  = pMove->raw.vPos;
        record.body.vVelocity  = pMove->raw.vStep;

        pPlayer->vLastPosition = pMove->raw.vPos;
        pPlayer->vLastVelocity = pMove->raw.vStep;

        pLock = &m_lock;

        EnterCriticalSection(pLock);
        pPlayer->EnqueueNetRecord(&record);
        LeaveCriticalSection(pLock);

        return;
    }

    if (pMove->nKind != PACKET_MOVING_EVENT)
    {
        return;
    }

    if (nLength != pEvents->nCount * 4 + 5)
    {
        // STRING: HYPERX 0x004bcc44
        g_gameApp.Error("MOVING_EVENT packet length %d instead of %d\n", nLength, pEvents->nCount * 4 + 5);

        return;
    }

    pPlayer                 = g_pGameSession->apPlayers[pMove->nSlot];
    record.nSlot            = pMove->nSlot;
    record.body.nSequence   = pPlayer->pState->nObjectSlot + nCarrierWord;
    record.body.nPlan       = pEvents->nPlan;
    record.body.nBehaviour  = pEvents->nBehaviour;
    int nEvent              = 0;
    record.body.vPosition   = pPlayer->vLastPosition;
    record.body.vVelocity   = pPlayer->vLastVelocity;
    pPlayer->cLastPlan      = pEvents->nPlan;
    pPlayer->cLastBehaviour = pEvents->nBehaviour;

    if (pEvents->nCount <= 0)
    {
        return;
    }

    pLock                      = &m_lock;
    const unsigned int* pEvent = pEvents->anWords;

    do
    {
        record.body.nCommand = *pEvent & ~EVENT_HAS_PARAM;

        if ((*pEvent & EVENT_HAS_PARAM) != 0)
        {
            nEvent++;
            pEvent++;
            record.body.nPayload = *pEvent;
        }
        else
        {
            record.body.nPayload = 0;
        }

        EnterCriticalSection(pLock);
        pPlayer->EnqueueNetRecord(&record);
        LeaveCriticalSection(pLock);

        nEvent++;
        pEvent++;
        record.body.nPlan = NO_SEQUENCE;
    } while (nEvent < pEvents->nCount);
}

// FUNCTION: HYPERX 0x0042ccc0
NetReceiveBuffer* PlayerBase::GetRecvBufBase()
{
    return &m_recvBuf;
}

// FUNCTION: HYPERX 0x0042ccd0
NetSendBuffer* PlayerBase::GetSendBufBase()
{
    return &m_sendBuf;
}

const unsigned int g_bMovePackingEnabled = 1;
