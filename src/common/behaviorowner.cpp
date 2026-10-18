#include "common/compat.h"
#include "common/behaviorowner.h"
#include "pooledclasses.h"

// FUNCTION: HYPERX 0x0041ed00
char BehaviorOwner::Insert(void* pObject)
{
    auto* const pItem = (GameBehaviorObject*)pObject;

    pItem->m_pStandby = m_pHead;
    m_pHead           = pItem;

    return 1;
}
