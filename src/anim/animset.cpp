#include "common/compat.h"
#include "animset.h"
#include "animdb.h"
#include "animregistry.h"
#include "common/fileobj.h"
#include "globals.h"
#include "planvm/planvm.h"
#include "common/pathstring.h"
#include <cstring>
#include <memory>
#include <cstdio>
#include "app/gameapp.h"
#include "animnode.h"

// Five kinds have their own offset; every other kind, zero included, returns one.
// FUNCTION: HYPERX 0x00401410
int AnimSet::GetFieldOffset(short nKind)
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

// FUNCTION: HYPERX 0x00417220
AnimClipSet* AnimSet::LoadIntoRegistry(FileObj* pFile, AnimRegistry* pRegistry, int, const char* pszName)
{
    auto& pSet = pRegistry->m_apSets[pRegistry->m_nSetCount];

    pSet = std::make_unique<AnimClipSet>(pFile, pszName);

    pRegistry->m_nSetCount++;

    return pSet.get();
}
