#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "planvm/agendaitem.h"
#include "planvm/cavoidobjects.h"
#include "globals.h"
#include "planvmglobals.h"
#include "engine/engineglobals.h"
#include "common/stub.h"
#include "engine/devicector.h"
#include "physics/physicsbody.h"
#include "engine/gameengine.h"
#include "physics/collisionplane.h"
#include "app/gameapp.h"
#include "physics/collisionellipsoid.h"

// FUNCTION: HYPERX 0x0040b930
void AgendaItem::OnListChanged()
{
    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->OnListChanged();
    }

    m_pNext = nullptr;
}

// FUNCTION: HYPERX 0x0040b990
char AgendaItem::Activate()
{
    const char  bInserted = 1;
    AgendaItem* pPrev     = nullptr;

    if (m_pOwner == nullptr)
    {
        OnListChanged();
        return bInserted;
    }

    AgendaItem* pNode = m_pOwner->GetObserver();

    while (pNode != nullptr && m_wKind > pNode->m_wKind)
    {
        pPrev = pNode;

        if (pNode->m_pOwner != m_pOwner)
        {
            pNode->m_pNext = nullptr;
            pNode->Detach();
        }

        pNode = pNode->m_pNext;
    }

    if (pNode == nullptr)
    {
        if (pPrev != nullptr)
        {
            pPrev->m_pNext = this;
            m_pNext        = nullptr;
            return bInserted;
        }

        m_pOwner->SetObserver(this);
        m_pNext = nullptr;
        return bInserted;
    }

    if (m_wKind == pNode->m_wKind && IsReplaceable() == 0)
    {
        if (pNode->m_bPersistent != 0)
        {
            if (pPrev != nullptr)
            {
                pPrev->m_pNext = pNode->m_pNext;
            }
            else
            {
                m_pOwner->SetObserver(pNode->m_pNext);
                pNode->m_pNext = nullptr;
            }
        }
        else
        {
            pNode->Detach();
        }
    }

    if (pPrev == nullptr)
    {
        m_pNext = m_pOwner->GetObserver();

        m_pOwner->SetObserver(this);
        return bInserted;
    }

    if (IsReplaceable() != 0)
    {
        if (m_wKind != Agenda_CAvoidObjects)
        {
            m_pNext        = pPrev->m_pNext;
            pPrev->m_pNext = this;
            return bInserted;
        }

        m_pOwner->GetObserver();

        CAvoidObjects* const pSwerve = static_cast<CAvoidObjects*>(this);

        if (FindByRef(pSwerve->m_pSwerveTarget) == 0)
        {
            pSwerve->m_pSwerveTarget = nullptr;
        }

        if (FindByRef(pSwerve->m_pDrawTarget) == 0)
        {
            pSwerve->m_pDrawTarget = nullptr;
        }

        if (pSwerve->m_pSwerveTarget == nullptr && pSwerve->m_pDrawTarget == nullptr)
        {
            // A virtual delete: slot zero is the real class's scalar deleting destructor.
            delete this;

            return 0;
        }
    }

    m_pNext        = pPrev->m_pNext;
    pPrev->m_pNext = this;
    return bInserted;
}

// An agenda item not in the list is notified twice, as in the original.

// FUNCTION: HYPERX 0x0040bb10
void AgendaItem::Detach()
{
    AgendaItem* pNode;

    if (m_pOwner == nullptr)
    {
        return;
    }

    AgendaItem* const pHead = m_pOwner->GetObserver();

    if (this == pHead)
    {
        m_pOwner->SetObserver(m_pNext);
    }
    else
    {
        for (pNode = pHead; pNode != nullptr; pNode = pNode->m_pNext)
        {
            if (this == pNode->m_pNext)
            {
                break;
            }
        }

        if (pNode != nullptr)
        {
            pNode->m_pNext = m_pNext;

            m_pNext = nullptr;
        }
        else
        {
            OnListChanged();
        }
    }

    OnListChanged();
}

// FUNCTION: HYPERX 0x0040bb80
void AgendaItem::BindWithNode(EngineObject*, GameBehaviorObject*, PlanNode*, int)
{
}

// FUNCTION: HYPERX 0x0040fea0
char AgendaItem::FindByRef(EngineObject* pObject)
{
    CAvoidObjects* pNode = (CAvoidObjects*)m_pOwner->GetObserver();

    while (pNode != nullptr)
    {
        if (pNode->m_wKind == Agenda_CBeAttached)
        {
            return 1;
        }

        if (pNode->m_wKind == Agenda_CAvoidObjects)
        {
            if (pNode->m_pSwerveTarget == pObject)
            {
                return 0;
            }

            if (pNode->m_pDrawTarget == pObject)
            {
                return 0;
            }
        }

        pNode = (CAvoidObjects*)pNode->m_pNext;
    }

    return 1;
}
