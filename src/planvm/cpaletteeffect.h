#ifndef HYPERBLADE_CPALETTEEFFECT_H
#define HYPERBLADE_CPALETTEEFFECT_H

#include "common/compat.h"
#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slot 0 is the shared deleting destructor; slot 8 shares CAvoidObjects::IsReplaceable's body.
// VTABLE: HYPERX 0x004b20ac
class CPaletteEffect : public ReleasingAgendaItem
{
public:
    CPaletteEffect()
    : m_pEffect(0)
    , m_nUnused(0)
    , m_nExpiry(0)
    {
    }

    virtual void Process(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);

    virtual char IsReplaceable()
    {
        return 1;
    }

    virtual void OnListChanged();

    // Its one call site allocates from g_apAgendaItemPools[Agenda_CPaletteEffect].
    int CreateVisualEffect(int nKind, int nAnim, PALETTEENTRY peStart, int nDuration);

    // The fade the tick drives and the engine clock reading it retires itself at.
    class PaletteFade* m_pEffect; // 0x10
    int                m_nUnused; // 0x14
    unsigned int       m_nExpiry; // 0x18
};

#endif
