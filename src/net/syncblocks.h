// The four blocks of one end of the asset sync, each reached by its place in the table.

#ifndef HYPERBLADE_SYNCBLOCKS_H
#define HYPERBLADE_SYNCBLOCKS_H

#include "net/syncblock.h"

// Each block's place in SyncBlocks, its asset type less ASSET_TYPE_SETTINGS.
enum SyncBlockIndex
{
    SYNC_BLOCK_SETTINGS,
    SYNC_BLOCK_TEAMS,
    SYNC_BLOCK_ARENAS,
    SYNC_BLOCK_PLANS,
    SYNC_BLOCK_COUNT
};

// GameSession::nAssetsArrived with a bit for every block.
#define SYNC_BLOCKS_ALL ((1 << SYNC_BLOCK_COUNT) - 1)

// Four 256-byte blocks of the asset verification buffer.
struct SyncBlocks
{
    SyncSettingsBlock* Settings() const
    {
        return static_cast<SyncSettingsBlock*>(apBlocks[SYNC_BLOCK_SETTINGS]);
    }

    SyncCatalogBlock* Teams() const
    {
        return static_cast<SyncCatalogBlock*>(apBlocks[SYNC_BLOCK_TEAMS]);
    }

    SyncCatalogBlock* Arenas() const
    {
        return static_cast<SyncCatalogBlock*>(apBlocks[SYNC_BLOCK_ARENAS]);
    }

    SyncCatalogBlock* Plans() const
    {
        return static_cast<SyncCatalogBlock*>(apBlocks[SYNC_BLOCK_PLANS]);
    }

    SyncBlockHeader* apBlocks[SYNC_BLOCK_COUNT];
};

#endif
