#ifdef HYPERX_V100

#include "common/compat.h"
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "math/vector.h"
#include "graphics/meshbone.h"
#include "graphics/transparentqueue.h"
#include "graphics/depthsort.h"

#define NODE_COUNT   0x400
#define BUCKET_COUNT 0x400

struct DepthNode
{
    DepthNode*        pNext;
    TransparentEntry* pEntry;
};

// Each bucket's list runs from the deepest entry to the nearest.
struct DepthBucket
{
    DepthNode* pHead;
    DepthNode* pTail;
    int        nCount;
};

// Read back as an int after the rounding constant is added, the double's low word is the rounded value.
union RoundedDouble
{
    double d;
    int    n;
};

namespace DepthSort
{

DepthNode*   s_pNodes;
DepthNode*   s_pFreeNode;
DepthNode*   s_pNodesEnd;
DepthNode*   s_pWalk;
DepthBucket* s_pBuckets;
DepthBucket* s_pBucket;

// 1.5 times two to the 52nd.
double s_dRoundConstant = 6755399441055744.0;

RoundedDouble s_deepest;
double        s_dNearest;

} // namespace DepthSort

// FUNCTION: HYPERX100 0x00408fd0
void __cdecl DepthSort::Init(void)
{
    s_pNodes    = (DepthNode*)malloc(NODE_COUNT * sizeof(DepthNode));
    s_pNodesEnd = s_pNodes + NODE_COUNT;
    s_pFreeNode = s_pNodes;
    memset(s_pNodes, 0, NODE_COUNT * sizeof(DepthNode));

    s_pBuckets = (DepthBucket*)malloc(BUCKET_COUNT * sizeof(DepthBucket));
    s_pBucket  = s_pBuckets;
    memset(s_pBuckets, 0, BUCKET_COUNT * sizeof(DepthBucket));

    s_dNearest  = 100000.0;
    s_deepest.d = -100000.0;
}

// FUNCTION: HYPERX100 0x00409060
void __cdecl DepthSort::Free(void)
{
    free(s_pNodes);
    free(s_pBuckets);

    s_pNodes    = 0;
    s_pNodesEnd = 0;
    s_pFreeNode = 0;
    s_pBuckets  = 0;
    s_pBucket   = 0;
}

// Keeps each bucket sorted by sort depth, deepest first. A full pool or a bucket past the end drops the entry.
// FUNCTION: HYPERX100 0x004090c0
void __cdecl DepthSort::Insert(TransparentEntry* pEntry, int nBucket)
{
    if (nBucket >= BUCKET_COUNT || s_pFreeNode == s_pNodesEnd)
    {
        return;
    }

    s_pBucket = &s_pBuckets[nBucket];

    s_pFreeNode->pEntry = pEntry;
    s_pFreeNode->pNext  = 0;

    if (s_pBucket->nCount == 0)
    {
        s_pBucket->pTail = s_pFreeNode;
        s_pFreeNode++;
        s_pBucket->pHead = s_pBucket->pTail;
        s_pBucket->nCount++;

        return;
    }

    s_pWalk = s_pBucket->pHead;

    if (s_pWalk->pEntry->flSortZ < pEntry->flSortZ)
    {
        s_pFreeNode->pNext = s_pWalk;
        s_pBucket->pHead   = s_pFreeNode;
        s_pFreeNode++;
        s_pBucket->nCount++;

        return;
    }

    for (s_pWalk = s_pWalk->pNext; s_pWalk != 0; s_pWalk = s_pWalk->pNext)
    {
        if (s_pWalk->pEntry->flSortZ < pEntry->flSortZ)
        {
            // The new node takes the walked node's entry and place, and the walked node takes the new entry.
            s_pFreeNode->pEntry = s_pWalk->pEntry;
            s_pWalk->pEntry     = pEntry;
            s_pFreeNode->pNext  = s_pWalk->pNext;

            if (s_pBucket->pTail == s_pWalk)
            {
                s_pBucket->pTail = s_pFreeNode;
            }

            s_pWalk->pNext = s_pFreeNode;
            s_pFreeNode++;
            s_pBucket->nCount++;

            return;
        }
    }

    s_pBucket->pTail->pNext = s_pFreeNode;
    s_pBucket->pTail        = s_pFreeNode;
    s_pFreeNode++;
    s_pBucket->nCount++;
}

// Draws the buckets from the deepest down, empties each, and resets the pool and the range.
// FUNCTION: HYPERX100 0x004092f0
void __cdecl DepthSort::DrawAll(void)
{
    s_deepest.d = s_deepest.d - s_dNearest;
    s_deepest.d = s_deepest.d + s_dRoundConstant;

    if (s_deepest.n >= BUCKET_COUNT)
    {
        s_deepest.n = BUCKET_COUNT - 1;
    }

    s_pBucket = &s_pBuckets[s_deepest.n];

    for (int i = 0; i <= s_deepest.n; i++)
    {
        if (s_pBucket->nCount > 0)
        {
            s_pFreeNode = s_pBucket->pHead;

            do
            {
                const Vec3* pLightDir;

                if (s_pFreeNode->pEntry->flWorldY > 0.0f)
                {
                    pLightDir = &g_lightDirs[1].dir;
                }
                else
                {
                    pLightDir = &g_lightDirs[2].dir;
                }

                g_lightDirs[0].dir = *pLightDir;

                s_pFreeNode->pEntry->pComponent->DrawFaces();

                s_pFreeNode = s_pFreeNode->pNext;
            } while (s_pFreeNode != 0);

            s_pBucket->nCount = 0;
            s_pBucket->pTail  = 0;
            s_pBucket->pHead  = s_pBucket->pTail;
        }

        s_pBucket--;
    }

    s_pFreeNode = s_pNodes;
    s_dNearest  = 1000000.0;
    s_deepest.d = -1000000.0;
}

#endif
