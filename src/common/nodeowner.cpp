#include "common/compat.h"
#include "planvm/interactor.h"
#include "common/nodeowner.h"

// FUNCTION: HYPERX 0x0046bd50
char NodeOwner::Insert(void* pNode)
{
    Interactor* const pItem = (Interactor*)pNode;

    pItem->pNext = m_pHead;
    m_pHead      = pItem;

    return 1;
}
