#ifndef HYPERBLADE_BASERESETAGENDAITEM_H
#define HYPERBLADE_BASERESETAGENDAITEM_H

#include "planvm/releasingagendaitem.h"

// The minimal slot nine, shared by eleven tables.
class BaseResetAgendaItem : public ReleasingAgendaItem
{
public:
    void OnListChanged() override;
};

#endif
