// Twelve bytes per entry, sorted on the depth key and held one-based.

#ifndef HYPERBLADE_TRANSPARENTQUEUE_H
#define HYPERBLADE_TRANSPARENTQUEUE_H

#include <memory>

struct TransparentEntry
{
    class MeshBone* pComponent;
    float           flSortZ;
    float           flWorldY;
};

// The entry a component builds before it is pushed.
// GLOBAL: HYPERX 0x004d0510
extern TransparentEntry g_renderPushEntry;

// GLOBAL: HYPERX 0x004cb5b8
extern TransparentEntry g_transparentSiftEntry;

// How many entries the queue was last built for, which is not the same as how many are in it.
// GLOBAL: HYPERX 0x004cb5c4
extern short g_nTransparentCapacity;

// GLOBAL: HYPERX 0x004cb5c8
extern std::unique_ptr<TransparentEntry[]> g_pTransparentQueue;

// GLOBAL: HYPERX 0x004cb5cc
extern short g_nTransparentCount;

#endif
