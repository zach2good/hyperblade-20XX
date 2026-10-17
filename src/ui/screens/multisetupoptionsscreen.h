#ifndef HYPERBLADE_MULTISETUPOPTIONSSCREEN_H
#define HYPERBLADE_MULTISETUPOPTIONSSCREEN_H

#include "common/compat.h"
#include "engine/game.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

// The first three copy UIScreen's confirm plate and buttons, at the same places.
enum MultiSetupOptionsWidget
{
    MULTI_SETUP_CONFIRM_PLATE,
    MULTI_SETUP_OKAY,
    MULTI_SETUP_CANCEL,
    MULTI_SETUP_TEXT_INPUT
};

// VTABLE: HYPERX 0x004b3d8c
class MultiSetupOptionsScreen : public Screen
{
public:
    MultiSetupOptionsScreen();
    ~MultiSetupOptionsScreen() override;

    void OnWidgetMessage(int nMsg, unsigned int nWidget) override;

    // Both go to the main menu.
    // FUNCTION: HYPERX 0x00479840
    virtual void OnOkay()
    {
        Game::SetEngineState(ESTATE_MAIN_MENU);
    }

    // The original folded this body onto OnOkay's, which carries the marker.
    virtual void OnCancel()
    {
        Game::SetEngineState(ESTATE_MAIN_MENU);
    }
};

// SYNTHETIC: HYPERX 0x00479810
// MultiSetupOptionsScreen::`scalar deleting destructor'

#endif
