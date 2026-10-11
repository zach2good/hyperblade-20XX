// Globals only the net files read; src/globals.cpp defines them in .data order.

#ifndef HYPERBLADE_NETGLOBALS_H
#define HYPERBLADE_NETGLOBALS_H
#include "common/compat.h"

#include "globals.h"
#include "net/syncblocks.h"
struct NetSessionEntry;

// One entry of the table the original's diagnostic calls GodsInventory.
struct GodsInventoryEntry
{
    int nObjectId;
    int nKind;
};

// Seven undecoded words cleared when a network game starts.
// GLOBAL: HYPERX 0x004d0970
extern int g_anNetworkSlotState[7];

// The index of the inventory's last entry.
#define GODS_INVENTORY_LAST 102

// GLOBAL: HYPERX 0x004c0ed0
extern GodsInventoryEntry g_godsInventory[GODS_INVENTORY_LAST + 1];

// GLOBAL: HYPERX 0x004d0790
extern UINT_PTR g_nNetSetupTimerId;

// Set while the dialog is filled programmatically, so its notifications are not user typing.
// GLOBAL: HYPERX 0x004d0794
extern int g_nNetSetupUpdating;

// GLOBAL: HYPERX 0x004d0798
extern int g_nNetSetupPendingSlot;

// The blocks the sync packet is built into, and the first of the heap block the teardown frees.
// GLOBAL: HYPERX 0x004d07a4
extern SyncBlocks g_syncSend;

// The blocks the remote end's sync packet arrives in, the second half of the same heap block.
// GLOBAL: HYPERX 0x004d07b4
extern SyncBlocks g_syncRecv;

// Set whenever the status buffer at g_szNetSetupStatus changes, so the dialog repaints it.
// GLOBAL: HYPERX 0x004d08c4
extern int g_bNetSetupStatusDirty;

// GLOBAL: HYPERX 0x004d07c4
extern char g_szNetSetupStatus[256];

// GLOBAL: HYPERX 0x004d077c
extern NetSessionEntry* g_pSessionList;

// GLOBAL: HYPERX 0x004d0780
extern int g_nSessionCount;

// The control the network setup dialog is currently validating.
// GLOBAL: HYPERX 0x004d079c
extern int g_nNetSetupControlId;

// GLOBAL: HYPERX 0x004c4fd0
extern int g_nDPlayLastError;

// Whether animation frames are stored compressed. Set at build time and never written.
// GLOBAL: HYPERX 0x004bcc10
extern const unsigned int g_bMovePackingEnabled;

// GLOBAL: HYPERX 0x004c1258
extern struct DPlayError g_aDPlayErrors[31];

#endif
