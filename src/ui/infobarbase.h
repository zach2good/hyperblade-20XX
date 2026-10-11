#ifndef HYPERBLADE_INFOBARBASE_H
#define HYPERBLADE_INFOBARBASE_H

#include "common/compat.h"
#include "graphics/renderer.h"

// The renderer interface: a destructor, one no-op and twenty three pure slots.
// SLOT TABLE 0x004b3068
// VTABLE: HYPERX 0x004b3068
class InfoBarBase : public Renderer
{
public:
    // Slot one is Object::Trace in the original, a shared free function.
    virtual void Unused()
    {
    }

    InfoBarBase()
    : m_bQuitPromptVisible(0)
    {
    }

    ~InfoBarBase()
    {
    }

    // src/app/gameapp.cpp reads the flag to route Enter and Esc.
    int          m_bQuitPromptVisible; // 0x004
    unsigned int m_nQuitPromptTick;    // 0x008

    static InfoBarBase* __cdecl Create(int nKind, HDC hDC);
};

// SYNTHETIC: HYPERX 0x0043e0d0
// InfoBarBase::`scalar deleting destructor'

#endif
