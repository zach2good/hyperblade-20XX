#ifndef HYPERBLADE_AWARDSUISCREEN_H
#define HYPERBLADE_AWARDSUISCREEN_H

#include "common/compat.h"
#include "ui/screens/matchinfouiscreen.h"

// VTABLE: HYPERX 0x004b22e0
class AwardsUIScreen : public MatchInfoUIScreen
{
public:
    AwardsUIScreen();

    void OnEnter();
    // Slot five: the case UIScreen::OnWidgetMessage's jump table does not answer by identifier.
    void OnOtherButton(int nMsg, unsigned int nId) override;
    void OnExitTabClick() override;

private:
    int m_nAward;      // 0x478, which award is showing, minus one for none
    int m_nAwardsView; // 0x47c, non-zero once the award panel has been built
};

#endif
