#include "common/compat.h"
#include "animclipentry.h"
#include "animdb.h"
#include "common/animnamedref.h"
#include "globals.h"
#include "app/gameapp.h"
#include "animnode.h"

// FUNCTION: HYPERX 0x004177c0
AnimClipEntry* AnimClipEntry::SyncCheck(AnimDB* pDb)
{
    auto* const pRef = m_pRef.get();

    pRef->m_pszName.reset();

    auto nNode = pRef->m_nSegment;

    if (nNode >= kAnimNamedRefBias)
    {
        nNode -= kAnimNamedRefBias;

        pRef->m_nSegment = nNode;
    }

    nNode = pRef->m_nSegment;

    if (nNode < pDb->GetNodeCount())
    {
        auto* const pNode = pDb->GetNode(nNode);

        if (pRef->m_nFrame <= pNode->m_wEndFrame)
        {
            pRef->m_pDb = pDb;

            return this;
        }
    }

    // STRING: HYPERX 0x004bb85c
    g_gameApp.Error("Sync Data< %s seg: %d frm: %d > is bad for assem: %s and will be left out\n",
                    pRef->m_pszName.get(),
                    nNode,
                    pRef->m_nFrame,
                    pDb->GetName());

    m_pRef.reset();

    return this;
}
