#ifndef HYPERBLADE_TRAININGENDSCREEN_H
#define HYPERBLADE_TRAININGENDSCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

// VTABLE: HYPERX 0x004b3160
class TrainingEndScreen : public Screen
{
public:
    void OnWidgetMessage(int nMsg, unsigned int nId) override;
    TrainingEndScreen();
    ~TrainingEndScreen() override;

private:
    int m_nSelectionMode; // 0x464
};

// SYNTHETIC: HYPERX 0x00443290
// TrainingEndScreen::`scalar deleting destructor'

#endif
