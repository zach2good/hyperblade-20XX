#include "common/compat.h"
#include "lodnode.h"
#include "graphics/modelcomponent.h"

// FUNCTION: HYPERX100 0x00477bb0
// FUNCTION: HYPERX 0x00472740
LODNode* LODNode::Init(short nLodCount)
{
    m_pRecord    = 0;
    m_wLodCount  = nLodCount;
    m_pMaterials = 0;
    m_wRefCount  = 0;
    m_pNext      = 0;

    memset(m_apLodMeshes, 0, sizeof(m_apLodMeshes));
    memset(m_aflFarther, 0, sizeof(m_aflFarther));
    memset(m_aflNearer, 0, sizeof(m_aflNearer));

    return this;
}

// Leaves the level count at minus one.

// FUNCTION: HYPERX100 0x00477c80
// FUNCTION: HYPERX 0x004727a0
LODNode::~LODNode()
{
    for (short nLevel = 0; nLevel < m_wLodCount; nLevel++)
    {
        for (short nComp = 0; nComp < m_pRecord->m_nSubComponents; nComp++)
        {
            if (m_apLodMeshes[nLevel][nComp] != 0)
            {
                delete m_apLodMeshes[nLevel][nComp];
            }
        }
    }

    while (m_wLodCount-- != 0)
    {
        delete[] m_apLodMeshes[m_wLodCount];
    }

    if (m_pRecord != 0)
    {
        delete m_pRecord;
    }
}

#ifdef HYPERX_V100
// FUNCTION: HYPERX100 0x00477c20
LODNode::LODNode()
{
    m_wLodCount  = 0;
    m_pRecord    = 0;
    m_wRefCount  = 0;
    m_pMaterials = 0;
    m_pNext      = 0;

    for (int i = 0; i < LOD_LEVEL_COUNT; i++)
    {
        m_apLodMeshes[i] = 0;
    }

    for (int j = 0; j < LOD_LEVEL_COUNT; j++)
    {
        m_aflFarther[j] = 0.0f;
        m_aflNearer[j]  = 0.0f;
    }
}
#endif
