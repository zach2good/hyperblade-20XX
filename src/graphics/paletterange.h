#ifndef HYPERBLADE_PALETTERANGE_H
#define HYPERBLADE_PALETTERANGE_H
#include "common/compat.h"

struct PaletteRange
{
    // Not POD: a by-value return would come back in EDX:EAX, and the callers carry an unwind state.
    PaletteRange()
    {
    }

    PaletteRange(int nNewFirst, int nNewCount)
    : nFirst(nNewFirst)
    , nCount(nNewCount)
    {
    }

    ~PaletteRange()
    {
    }

    void Set(int nNewFirst, int nNewCount)
    {
        nFirst = nNewFirst;
        nCount = nNewCount;
    }

    int Holds(int nIndex) const
    {
        return nIndex >= nFirst && nIndex <= nCount;
    }

    int nFirst;
    int nCount;
};

#endif
