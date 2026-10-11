#include "common/compat.h"
#include "animdb.h"
#include "common/animnamedref.h"
#include "animclipentry.h"
#include "animnode.h"

#include "engine/messageids.h"

// FUNCTION: HYPERX 0x00417530
AnimDB::AnimDB(const char* pszName, const char*)
: m_pOwnedBlock(0)
, m_wNodeCount(0)
, m_nEntryCount(0)
{
    memset(m_apNodes, 0, sizeof(m_apNodes));
    memset(m_apEntries, 0, sizeof(m_apEntries));

    SetName(pszName);
}

// An entry owns its m_pRef; the database owns m_pOwnedBlock.

// FUNCTION: HYPERX 0x004175c0
AnimDB::~AnimDB()
{
    if (m_pOwnedBlock != 0)
    {
        ::operator delete(m_pOwnedBlock);

        m_pOwnedBlock = 0;
    }

    for (short nEntry = 0; nEntry < m_nEntryCount; nEntry++)
    {
        AnimClipEntry* const pEntry = m_apEntries[nEntry];

        if (pEntry != 0)
        {
            delete pEntry->m_pRef;

            delete pEntry;
        }

        m_apEntries[nEntry] = 0;
    }

    for (short nNode = 0; nNode < m_wNodeCount; nNode++)
    {
        AnimNode* const pNode = m_apNodes[nNode];

        if (pNode != 0)
        {
            if (pNode->m_pszDescription != 0)
            {
                delete[] pNode->m_pszDescription;

                pNode->m_pszDescription = 0;
            }

            delete pNode;

            m_apNodes[nNode] = 0;
        }
    }
}

// FUNCTION: HYPERX 0x004176b0
void AnimDB::SetName(const char* pszName)
{
    if (pszName != 0)
    {
        lstrcpynA(m_szName, pszName, sizeof(m_szName));

        return;
    }

    m_szName[0] = 0;
}

// The slot is written even when the allocation failed; only success advances the count.
// FUNCTION: HYPERX 0x004176d0
void AnimDB::AddClipEntry(unsigned short wSegment, unsigned short wTriggerFrame, const char* pszName, int nMessage, const char* pszRef, short nSyncSegment, short nSyncFrame)
{
    AnimClipEntry* const pEntry = new AnimClipEntry(wSegment, wTriggerFrame, pszName, nMessage);

    m_apEntries[m_nEntryCount] = pEntry;

    if (pEntry != 0)
    {
        if (nMessage == MSG_SYNC_FRAME)
        {
            m_apEntries[m_nEntryCount]->m_pRef = new AnimNamedRef(pszRef, nSyncSegment, nSyncFrame);
        }

        m_nEntryCount++;
    }
}
