#ifndef HYPERBLADE_CDOPPLERSND_H
#define HYPERBLADE_CDOPPLERSND_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slots 0 and 2 are bodies many tables share, declared where they are defined.
// VTABLE: HYPERX 0x004b2044
class CDopplerSnd : public ReleasingAgendaItem
{
public:
    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    // Zeroed by Bind and OnListChanged and read by nothing in the image.
    int m_nUnusedA; // 0x10
    int m_nUnusedB; // 0x14
    int m_nUnusedC; // 0x18
};

#endif
