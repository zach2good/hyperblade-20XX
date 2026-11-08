#include "common/compat.h"
#include "animset.h"
#include "animdb.h"
#include "animregistry.h"
#include "common/fileobj.h"
#include "globals.h"
#include "planvm/planvm.h"
#include "common/pathstring.h"
#include <string.h>
#include <new.h>
#include <stdio.h>
#include "app/gameapp.h"
#include "animnode.h"

// Five kinds have their own offset; every other kind, zero included, returns one.
// FUNCTION: HYPERX100 0x00469260
// FUNCTION: HYPERX 0x00401410
int __cdecl AnimSet::GetFieldOffset(short nKind)
{
    switch (nKind)
    {
        case 1:
            return 4;

        case 2:
            return 7;

        case 3:
            return 10;

        case 4:
            return 13;

        case 0:
            return 1;

        case 5:
            return 16;
    }

    return 1;
}

// A failed allocation files a null, so the cache's count advances either way.
// FUNCTION: HYPERX100 0x00459cc0
// FUNCTION: HYPERX 0x00417220
AnimClipSet* __cdecl AnimSet::LoadIntoRegistry(FileObj* pFile, AnimRegistry* pRegistry, int, const char* pszName)
{
    AnimClipSet* const pSet = new AnimClipSet(pFile, pszName);

    pRegistry->m_apSets[pRegistry->m_nSetCount] = pSet;

    pRegistry->m_nSetCount++;

    return pSet;
}

#ifdef HYPERX_V100
// FUNCTION: HYPERX100 0x004692d0
short __cdecl AnimSet::GetKindFromOffset(int nOffset)
{
    switch (nOffset)
    {
        case 1:
            return 0;

        case 4:
            return 1;

        case 7:
            return 2;

        case 10:
            return 3;

        case 13:
            return 4;

        case 16:
            return 5;
    }

    return 0;
}
#endif
