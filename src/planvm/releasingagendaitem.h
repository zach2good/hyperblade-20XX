#ifndef HYPERBLADE_RELEASINGAGENDAITEM_H
#define HYPERBLADE_RELEASINGAGENDAITEM_H

#include "planvm/kindagendaitem.h"

// Slot two in twenty three tables: passes the release down the 0x0c link.
class ReleasingAgendaItem : public KindAgendaItem
{
public:
    void Release(int nArg) override;
};

#endif
