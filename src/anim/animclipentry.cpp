#include "common/compat.h"
#include "animclipentry.h"
#include "animdb.h"
#include "common/animnamedref.h"
#include "globals.h"
#include <new>
#include "app/gameapp.h"
#include "animnode.h"

// FUNCTION: HYPERX 0x004177c0
AnimClipEntry* AnimClipEntry::SyncCheck(AnimDB* pDb)
{
    AnimNamedRef* pRef = m_pRef;

    if (pRef->m_pszName != 0)
    {
        delete[] pRef->m_pszName;

        pRef->m_pszName = 0;
    }

    pRef = m_pRef;

    short nNode = pRef->m_nSegment;

    if (nNode >= ANIMNAMEDREF_BIAS)
    {
        nNode -= ANIMNAMEDREF_BIAS;

        pRef->m_nSegment = nNode;
    }

    pRef = m_pRef;

    nNode = pRef->m_nSegment;

    if (nNode < pDb->GetNodeCount())
    {
        AnimNode* const pNode = pDb->GetNode(nNode);

        if (pRef->m_nFrame <= pNode->m_wEndFrame)
        {
            pRef->m_pDb = pDb;

            return this;
        }
    }

    // STRING: HYPERX 0x004bb85c
    g_gameApp.Error("Sync Data< %s seg: %d frm: %d > is bad for assem: %s and will be left out\n",
                    pRef->m_pszName,
                    nNode,
                    pRef->m_nFrame,
                    pDb->GetName());

    pRef = m_pRef;

    if (pRef->m_pszName != 0)
    {
        delete[] pRef->m_pszName;

        pRef->m_pszName = 0;
    }

    pRef = m_pRef;

    delete pRef;

    m_pRef = 0;

    return this;
}
