// The blocks a sync packet is built from, packed to one for the wire.

#ifndef HYPERBLADE_SYNCBLOCK_H
#define HYPERBLADE_SYNCBLOCK_H

#include "common/staticassert.h"
#include "net/netmessage.h"

// Each block has a slot of this many bytes, which bounds its open-ended array.
#define SYNC_BLOCK_BYTES 0x100

#pragma pack(push, 1)

// What every block starts with, read before the kind says which block it is.
struct SyncBlockHeader : NetMessage
{
    // A block nothing has filled.
    int IsEmpty() const
    {
        return (nKind | nCount) == 0;
    }

    unsigned char nCount; // 0x01
};

// A file's write time as its low and high halves, and its size.
struct SyncFileStamp
{
    unsigned short wTimeLow;  // 0x00
    unsigned short wTimeHigh; // 0x02
    unsigned short wSize;     // 0x04
};

struct SyncSettingsBlock : SyncBlockHeader
{
    int            nVersion;    // 0x02
    unsigned char  nDifficulty; // 0x06
    unsigned char  nBrutality;  // 0x07
    unsigned short nPeriod;     // 0x08
    unsigned char  nArena;      // 0x0a
    unsigned char  nTeamOne;    // 0x0b
    unsigned char  nTeamTwo;    // 0x0c
    unsigned char  nSlotCount;  // 0x0d
    SyncFileStamp  enumIds;     // 0x0e
    SyncFileStamp  teams;       // 0x14
    SyncFileStamp  plans;       // 0x1a
    SyncFileStamp  common;      // 0x20
    // The loop fills nCount and pads the rest.
    unsigned char aSlots[SYNC_BLOCK_BYTES - 0x26]; // 0x26
};

// Every kind but the settings one: the count, then a stamp per entry.
struct SyncCatalogBlock : SyncBlockHeader
{
    SyncFileStamp aStamps[(SYNC_BLOCK_BYTES - 2) / 6]; // 0x02
};

#pragma pack(pop)

STATIC_ASSERT(sizeof(SyncFileStamp) == 6);
STATIC_ASSERT(sizeof(SyncBlockHeader) == 2);
STATIC_ASSERT(sizeof(SyncSettingsBlock) == SYNC_BLOCK_BYTES);
STATIC_ASSERT(sizeof(SyncCatalogBlock) <= SYNC_BLOCK_BYTES);
#endif
