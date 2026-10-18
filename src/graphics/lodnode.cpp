#include "common/compat.h"
#include "lodnode.h"
#include "graphics/modelcomponent.h"
#include <cstring>

// FUNCTION: HYPERX 0x00472740
LODNode* LODNode::Init(short nLodCount)
{
    m_pRecord    = nullptr;
    m_wLodCount  = nLodCount;
    m_pMaterials = nullptr;
    m_wRefCount  = 0;
    m_pNext      = nullptr;

    memset(m_apLodMeshes, 0, sizeof(m_apLodMeshes));
    memset(m_aflFarther, 0, sizeof(m_aflFarther));
    memset(m_aflNearer, 0, sizeof(m_aflNearer));

    return this;
}

// Leaves the level count at minus one.

// FUNCTION: HYPERX 0x004727a0
LODNode::~LODNode()
{
    for (short nLevel = 0; nLevel < m_wLodCount; nLevel++)
    {
        for (short nComp = 0; nComp < m_pRecord->m_nSubComponents; nComp++)
        {
            if (m_apLodMeshes[nLevel][nComp] != nullptr)
            {
                delete m_apLodMeshes[nLevel][nComp];
            }
        }
    }

    while (m_wLodCount-- != 0)
    {
        delete[] m_apLodMeshes[m_wLodCount];
    }

    if (m_pRecord != nullptr)
    {
        delete m_pRecord;
    }
}
