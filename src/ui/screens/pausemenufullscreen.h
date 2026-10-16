#ifndef HYPERBLADE_PAUSEMENUFULLSCREEN_H
#define HYPERBLADE_PAUSEMENUFULLSCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

// In build order.
enum PauseFullWidget
{
    PAUSE_FULL_SAVE,
    PAUSE_FULL_OPTIONS,
    PAUSE_FULL_EXIT,
    PAUSE_FULL_RETURN,
    PAUSE_FULL_QUIT
};

// VTABLE: HYPERX 0x004b316c
class PauseMenuFullScreen : public Screen
{
public:
    virtual void OnWidgetMessage(int nMsg, unsigned int nId);
    PauseMenuFullScreen();
    ~PauseMenuFullScreen();
    void Init();

private:
    int m_nUnusedD; // 0x464
};

static_assert(sizeof(PauseMenuFullScreen) == 0x468);

// SYNTHETIC: HYPERX 0x004433d0
// PauseMenuFullScreen::`scalar deleting destructor'

#endif
