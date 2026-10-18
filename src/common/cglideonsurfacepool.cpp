#include "common/compat.h"
#include "common/cglideonsurfacepool.h"
#include "planvm/agendaitem.h"
#include "planvm/cglideonsurface.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00412b80
void CGlideOnSurfacePool::Init()
{
    AgendaItem* pNode = new CGlideOnSurface;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountCGlideOnSurface);

    while (nRemaining-- > 0)
    {
        pNode = new CGlideOnSurface;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00412c10
void* CGlideOnSurfacePool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CGlideOnSurface;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00414df0
CGlideOnSurfacePool::~CGlideOnSurfacePool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
