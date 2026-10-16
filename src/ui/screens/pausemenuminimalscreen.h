#ifndef HYPERBLADE_PAUSEMENUMINIMALSCREEN_H
#define HYPERBLADE_PAUSEMENUMINIMALSCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

// In build order.
enum PauseMinimalWidget
{
    PAUSE_MINIMAL_RETURN,
    PAUSE_MINIMAL_QUIT
};

// VTABLE: HYPERX 0x004b3178
class PauseMenuMinimalScreen : public Screen
{
public:
    virtual void OnWidgetMessage(int nMsg, unsigned int nId);
    PauseMenuMinimalScreen();
    ~PauseMenuMinimalScreen();
    void Init();

private:
    int m_nUnusedD; // 0x464
};

static_assert(sizeof(PauseMenuMinimalScreen) == 0x468);

// SYNTHETIC: HYPERX 0x00443960
// PauseMenuMinimalScreen::`scalar deleting destructor'

#endif
