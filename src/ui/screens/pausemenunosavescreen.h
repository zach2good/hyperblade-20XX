#ifndef HYPERBLADE_PAUSEMENUNOSAVESCREEN_H
#define HYPERBLADE_PAUSEMENUNOSAVESCREEN_H

#include "common/compat.h"
#include "common/staticassert.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

// In build order.
enum PauseNoSaveWidget
{
    PAUSE_NO_SAVE_OPTIONS,
    PAUSE_NO_SAVE_EXIT,
    PAUSE_NO_SAVE_RETURN,
    PAUSE_NO_SAVE_QUIT
};

// VTABLE: HYPERX 0x004b3184
class PauseMenuNoSaveScreen : public Screen
{
public:
    virtual void OnWidgetMessage(int nMsg, unsigned int nId);
    PauseMenuNoSaveScreen();
    ~PauseMenuNoSaveScreen();
    void Init();

private:
    int m_nUnusedD; // 0x464
};

STATIC_ASSERT(sizeof(PauseMenuNoSaveScreen) == 0x468);

// SYNTHETIC: HYPERX 0x00443c90
// PauseMenuNoSaveScreen::`scalar deleting destructor'

#endif
