#ifndef HYPERBLADE_NETRECORDS_H
#define HYPERBLADE_NETRECORDS_H

#include "math/vector.h"
#include "net/movecodec.h"
#include "net/playerslotname.h"
#include "net/netmessage.h"

// The words that fit after an events packet's five header bytes.
constexpr auto kEventWordsMax = 0x3e;

constexpr auto kNetDataMax = 0x80;

// Laid out on wire boundaries; the packing is part of the type.
#pragma pack(push, 1)

// A game message that carries the sender's slot after its kind.
struct NetMessageHeader : NetMessage
{
    unsigned char nSlot;        // 0x01
    char          szPayload[1]; // 0x02
};

// A player announcing its slot: the header, then the name record.
struct NetPlayerNameMessage : NetMessage
{
    unsigned char  nSlot; // 0x01
    PlayerSlotName name;  // 0x02
};

// A message that is its kind and one byte.
struct NetShortMessage : NetMessage
{
    unsigned char nValue; // 0x01
};

// What the receiver copies out of a record and applies.
struct NetMoveBody
{
    unsigned int nCommand;       // 0x00
    unsigned int nPayload;       // 0x04
    unsigned int nSecondCommand; // 0x08
    unsigned int nSecondPayload; // 0x0c
    // The sender's kind in the low word; above it, a target slot or a camera plus one.
    int          nSequence;  // 0x10
    unsigned int nPlan;      // 0x14
    unsigned int nBehaviour; // 0x18

    union
    {
        struct
        {
            Vec3 vPosition; // 0x1c
            Vec3 vVelocity; // 0x28
        };

        // A static record carries a nibble per stationary object here instead.
        char abNibbles[24];
    };
};

struct NetMoveRecord
{
    unsigned char cKind; // 0x00
    unsigned char nSlot; // 0x01
    unsigned int  nTime; // 0x02
    NetMoveBody   body;  // 0x06
};

// A move: the kind byte says whether a packed move or the raw position and step follow.
struct MovePacket : NetMessage
{
    unsigned char nSlot;

    union
    {
        PackedMove packed;

        struct
        {
            Vec3 vPos;
            Vec3 vStep;
        } raw;
    };
};

// A word per event, and a second word after any event that carries a parameter.
struct EventsPacket : NetMessage
{
    unsigned char nSlot;
    unsigned char nPlan;
    unsigned char nBehaviour;
    unsigned char nCount;
    unsigned int  anWords[kEventWordsMax];
};

// The stationary objects' packet: its kind, how many event bytes follow the bitmap, and a nibble
// per stationary object.
struct StationaryPacket : NetMessage
{
    unsigned char nEventBytes;
    unsigned char abSlotNibbles[1];
};

// A stationary event's command, and the payload only when the command carries its long flag,
// so an event without one ends at nPayload.
struct StationaryCommand
{
    unsigned int nCommand;
    unsigned int nPayload;
};

// A stationary object's event, after the nibbles: the slot, then its command.
struct StationaryEvent
{
    unsigned char     nSlot;
    StationaryCommand command;
};

// A data message: its kind, the payload's length, and the payload with its terminator.
struct NetDataMessage : NetMessage
{
    unsigned char nLength;
    char          abData[kNetDataMax + 1];
};

#pragma pack(pop)

static_assert(sizeof(StationaryEvent) == 9);

#endif
