#ifndef HYPERBLADE_CPLAYER_H
#define HYPERBLADE_CPLAYER_H

#include "common/compat.h"
#include "net/playerbase.h"
#include "net/session.h"
#include "net/directplay.h"
#include "net/netrecords.h"

// VTABLE: HYPERX 0x004b2b80
class CPlayer : public PlayerBase
{
public:
    static unsigned int NetworkReceiveThreadProc(class CPlayer* pPlayer);

public:
    // Its thread proc is CPlayer::NetworkReceiveThreadProc.
    HANDLE StartNetworkThread(HANDLE hEvent);

    virtual ~CPlayer();
    explicit CPlayer(GameSession* pSession);
    // The one slot the base leaves pure.
    virtual int SendPacket(unsigned long idTo, NetMessage* pMessage, unsigned long dwDataSize);

    void SendDataMsg(unsigned long idTo, const void* pData, int nLength);
    void SendRawMsg(unsigned long idTo, NetMessage* pMessage, int nLength);
    void SendStringMsg(unsigned long idTo, int nValue, const char* pszText);
    int  SendDropPlayer(unsigned long idTo);

    // The five two-byte messages: a kind byte and a second byte.
    void SendHostStart(unsigned long idTo);
    void SendPlayerReady(unsigned long idTo);
    // A word, not a byte: its one caller passes a masked slot.
    void SendPlayerConnected(unsigned long idTo, int nValue);
    void SendSlotAccepted(unsigned long idTo, char cValue);
    void SendSlotRejected(unsigned long idTo, char cValue);
    // The sixth, whose second byte is never set.
    void SendAssetRequest(unsigned long idTo);

    void AssignObject(int nObject);

    int          m_nSlot; // 0x26c
    PlayerState* pState;  // 0x270
    // The last move received, which a moving event reuses.
    Vec3 vLastPosition;  // 0x274
    Vec3 vLastVelocity;  // 0x280
    char cLastPlan;      // 0x28c
    char cLastBehaviour; // 0x28d
    // The block pState points at, embedded rather than allocated.
    PlayerState  stateBlock; // 0x290
    IDirectPlay* pDPlay;     // 0x2f8
    // The event the receive thread waits on.
    HANDLE        hPacketEvent; // 0x2fc
    int           nUnusedB;     // 0x300
    unsigned long nThreadId;    // 0x304
    // The constructor sets it to one, and nothing reads it.
    int nUnusedC; // 0x308
    // The send flags SendPacket passes to DirectPlay.
    int nSendFlags; // 0x30c
    // Two at construction, and read as the receive flags by the dispatch.
    int nRecvFlags; // 0x310
    // A ring of received records; a full ring drops its oldest.
    unsigned int  nQueueCount; // 0x314
    unsigned int  nQueueHead;  // 0x318
    NetMoveRecord aQueue[150]; // 0x31c

    char           EnqueueNetRecord(const NetMoveRecord* pRecord);
    NetMoveRecord* DequeueNetRecord();

    void DetachObject();

    unsigned int DispatchPacket();

    unsigned int NetworkReceiveLoop();

    void ResetState();

    int StopNetworkThread();
};

// SYNTHETIC: HYPERX 0x0042d350
// CPlayer::`scalar deleting destructor'

#endif
