#ifndef HYPERBLADE_INFOBARBASE_H
#define HYPERBLADE_INFOBARBASE_H

#include "common/compat.h"
#include "common/clock.h"
#include "graphics/renderer.h"

// The renderer interface: a destructor, one no-op and twenty three pure slots.
// SLOT TABLE 0x004b3068
// VTABLE: HYPERX 0x004b3068
class InfoBarBase : public Renderer
{
public:
    // Slot one is Object::Trace in the original, a shared free function.
    void Unused() override
    {
    }

    InfoBarBase()
    : m_bQuitPromptVisible(0)
    {
    }

    ~InfoBarBase() override = default;

    // src/app/gameapp.cpp reads the flag to route Enter and Esc.
    int                 m_bQuitPromptVisible; // 0x004
    Clock::Milliseconds m_quitPromptTick;     // 0x008

    static InfoBarBase* Create();
};

// SYNTHETIC: HYPERX 0x0043e0d0
// InfoBarBase::`scalar deleting destructor'

#endif
