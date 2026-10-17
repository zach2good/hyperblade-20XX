#ifndef HYPERBLADE_KEYCONFIGSCREEN_H
#define HYPERBLADE_KEYCONFIGSCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

// VTABLE: HYPERX 0x004b2c88
class KeyConfigScreen : public Screen
{
public:
    // Slot zero here, not the base's shared stub: the answer is the row it bound.
    int  OnKeyMessage(unsigned int nKeyCode, unsigned int nUnused) override;
    void OnWidgetMessage(int nMsg, unsigned int nWidget) override;

    KeyConfigScreen();
    ~KeyConfigScreen() override;

    // Inlined in the original; only its -1 survives as a comparison.
    int FindActiveRow();

private:
    // Minus one when no row is waiting. The original allocates 0x468 bytes.
    int m_nActiveRemap; // 0x464
};

// SYNTHETIC: HYPERX 0x0042fc10
// KeyConfigScreen::`scalar deleting destructor'

#endif
