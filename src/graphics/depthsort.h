#ifdef HYPERX_V100

// v1.0.0.0's unused depth-bucket sort for transparent components. Nothing calls it, and nothing
// sets the depth range it reads.
#ifndef HYPERBLADE_DEPTHSORT_H
#define HYPERBLADE_DEPTHSORT_H

struct TransparentEntry;

namespace DepthSort
{

void __cdecl Init(void);
void __cdecl Free(void);
void __cdecl Insert(TransparentEntry* pEntry, int nBucket);
void __cdecl DrawAll(void);

} // namespace DepthSort

#endif

#endif
