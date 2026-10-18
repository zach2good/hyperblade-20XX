#ifndef HYPERBLADE_CONDITIONTABLE_H
#define HYPERBLADE_CONDITIONTABLE_H

#include "planvm/agendaitemids.h"
#include "common/pooledownerroot.h"

class AgendaItem;

// One pool per agenda item, built by GameEngine::CreateAgendaItemPools, then a null.
// GLOBAL: HYPERX 0x004cb43c
extern PooledOwnerRoot* g_apAgendaItemPools[Agenda_Count + 1];

// Every pool in the table hands out agenda items.
inline AgendaItem* AllocAgendaItem(int nKind)
{
    return static_cast<AgendaItem*>(g_apAgendaItemPools[nKind]->Alloc());
}

#endif
