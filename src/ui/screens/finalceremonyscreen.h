#ifndef HYPERBLADE_FINALCEREMONYSCREEN_H
#define HYPERBLADE_FINALCEREMONYSCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

enum FinalCeremonyWidget
{
    FINAL_CEREMONY_LOGO,
    FINAL_CEREMONY_ANIMATION
};

// 0x00442fc0 makes no base constructor call, so the base is Screen.
// SLOT TABLE 0x004b3158
// VTABLE: HYPERX 0x004b3154
class FinalCeremonyScreen : public Screen
{
public:
    void OnWidgetMessage(int nMsg, unsigned int nId) override;
    FinalCeremonyScreen();
    ~FinalCeremonyScreen() override;
};

// SYNTHETIC: HYPERX 0x00443170
// FinalCeremonyScreen::`scalar deleting destructor'

#endif
