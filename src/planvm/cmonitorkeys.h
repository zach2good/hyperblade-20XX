#ifndef HYPERBLADE_CMONITORKEYS_H
#define HYPERBLADE_CMONITORKEYS_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// DATA/ENUMIDS.H's KeyMonitorModes. Nothing sets one but the first: id_SET_KeyMonitorMode is empty.
#define MON_DRIVE_YER_GUY 0
#define MON_GOALIE_CTRL   1
#define MON_ARCADE_STYLE  2

// Slots 0 and 2 are bodies many tables share, declared where they are defined.
// VTABLE: HYPERX 0x004b1e70
class CMonitorKeys : public ReleasingAgendaItem
{
public:
    CMonitorKeys()
    {
        m_nMode = MON_DRIVE_YER_GUY;
    }

    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    // The owner body's two input words. Bind takes their addresses.
    short* m_pInput;     // 0x10, INTERACTOR_INPUT
    short* m_pPrevInput; // 0x14, INTERACTOR_PREV_INPUT
    int    m_nUnused;    // 0x18
    int    m_nMode;      // 0x1c
};

// The original folds many agenda items' deleters onto this one.
// SYNTHETIC: HYPERX 0x004142f0
// CMonitorKeys::`scalar deleting destructor'

#endif
