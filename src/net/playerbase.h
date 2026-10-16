#ifndef HYPERBLADE_PLAYERBASE_H
#define HYPERBLADE_PLAYERBASE_H

#include "common/compat.h"
#include "engine/devicector.h"
#include "globals.h"
#include "net/netreceivebuffer.h"
#include "net/netsendbuffer.h"
#include <mutex>

class EngineObject;

// The send buffer's payload starts past a six-byte header.
#define NET_SEND_HEADER_BYTES 6

// Abstract: slot ten is _purecall here and CPlayer::SendPacket in the derived table.
// SLOT TABLE 0x004b2b2c
// VTABLE: HYPERX 0x004b2b2c
class PlayerBase
{
public:
    virtual ~PlayerBase();

    // Answers what the slot the caller read this from should hold afterwards.
    // The original's linker folded this body onto PooledOwnerRoot::Destroy, which carries the marker.
    virtual PlayerBase* DestroyViaVtable()
    {
        delete this;

        return 0;
    }

    virtual void              InitDirectPlayParams();
    virtual void              NetUpdateObject(unsigned char nSlot);
    virtual void              NetUpdateMultiObject(int nUnused);
    virtual void              RecvStaticObjectPacket(const NetReceiveBuffer* pPacket, int nLength);
    virtual void              RecvMovementPacket(const NetReceiveBuffer* pPacket, int nLength);
    virtual NetReceiveBuffer* GetRecvBufBase();
    virtual NetSendBuffer*    GetSendBufBase();

    // FUNCTION: HYPERX 0x0042cce0
    virtual unsigned char* GetSendPayloadPtr()
    {
        return GetSendBufBase()->aBytes + NET_SEND_HEADER_BYTES;
    }

    virtual int __fastcall SendPacket(unsigned long idTo, NetMessage* pMessage, unsigned long dwDataSize) = 0;

    void __fastcall BroadcastState(NetSendBuffer* pPacket, int nLength, int nObjectSlot);

    int nUnusedA; // 0x004
    // The object this player drives.
    EngineObject* pObject; // 0x008
    // The physics body of the object above.
    PhysicsBody*      pBody;    // 0x00c
    NetReceiveBuffer* pBufferA; // 0x010
    NetSendBuffer*    pBufferB; // 0x014
    // Six bytes into the send buffer; nothing reads it back.
    unsigned char* pSendPayload; // 0x018
    Vec3           m_vPrevPos;   // 0x01c
    Vec3           m_vPrevVel;   // 0x028
    // Carried over from the previous send.
    unsigned char m_nPrevPlan;      // 0x034
    unsigned char m_nPrevBehaviour; // 0x035
    int           m_nLastSend;      // 0x038

    NetReceiveBuffer m_recvBuf; // 0x03c
    NetSendBuffer    m_sendBuf; // 0x13c
    // m_dplayLock serialises the DirectPlay sends and receives; m_lock guards the applied object state.
    std::recursive_mutex m_dplayLock; // 0x23c
    std::recursive_mutex m_lock;      // 0x254
};

// SYNTHETIC: HYPERX 0x0042ccf0
// PlayerBase::`scalar deleting destructor'

#endif
