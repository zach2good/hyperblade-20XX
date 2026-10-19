#ifndef HYPERBLADE_CPALETTEEFFECT_H
#define HYPERBLADE_CPALETTEEFFECT_H

#include <memory>

#include "common/compat.h"
#include "common/clock.h"
#include "graphics/bitmapformat.h"
#include "graphics/palettefade.h"
#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slot 0 is the shared deleting destructor; slot 8 shares CAvoidObjects::IsReplaceable's body.
// VTABLE: HYPERX 0x004b20ac
// The four effect kinds the jump table covers, three of them scalar.
enum class EffectKind
{
    kScalarA = 1,
    kPlain   = 2,
    kScalarB = 3,
    kScalarC = 4,
};

class CPaletteEffect : public ReleasingAgendaItem
{
public:
    CPaletteEffect()
    : m_pEffect(nullptr)
    , m_nUnused(0)
    , m_expiry(0)
    {
    }

    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;

    char IsReplaceable() override
    {
        return 1;
    }

    void OnListChanged() override;

    // Its one call site allocates from g_apAgendaItemPools[Agenda_CPaletteEffect].
    int CreateVisualEffect(EffectKind kind, int nAnim, PaletteEntry peStart, int nDuration);

    // The fade the tick drives and the engine clock reading it retires itself at.
    std::unique_ptr<PaletteFade> m_pEffect; // 0x10
    int                          m_nUnused; // 0x14
    Clock::Milliseconds          m_expiry;  // 0x18
};

#endif
