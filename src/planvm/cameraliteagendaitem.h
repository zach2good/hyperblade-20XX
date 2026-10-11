#ifndef HYPERBLADE_CAMERALITEAGENDAITEM_H
#define HYPERBLADE_CAMERALITEAGENDAITEM_H

#include "planvm/releasingagendaitem.h"

// The other shared slot nine, held only by the two camera tables.
class CameraLiteAgendaItem : public ReleasingAgendaItem
{
public:
    CameraLiteAgendaItem()
    {
    }

    CameraLiteAgendaItem(Vec3* pPos, Vec3* pDir)
    : m_pWatchPos(pPos)
    , m_pWatchDir(pDir)
    {
    }

    virtual void OnListChanged();

    Vec3* m_pWatchPos; // 0x10
    Vec3* m_pWatchDir; // 0x14
};

#endif
