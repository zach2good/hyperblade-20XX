#ifndef HYPERBLADE_GAMEINTROSCREEN_H
#define HYPERBLADE_GAMEINTROSCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

// A screen whose whole construction is picking a background bitmap.
// VTABLE: HYPERX 0x004b3200
class GameIntroScreen : public Screen
{
public:
    GameIntroScreen();
    ~GameIntroScreen() override;
    void OnWidgetMessage(int nMsg, unsigned int nWidget) override;
};

// SYNTHETIC: HYPERX 0x00449190
// GameIntroScreen::`scalar deleting destructor'

#endif
