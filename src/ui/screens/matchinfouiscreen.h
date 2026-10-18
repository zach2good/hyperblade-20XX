#ifndef HYPERBLADE_MATCHINFOUISCREEN_H
#define HYPERBLADE_MATCHINFOUISCREEN_H

#include "common/compat.h"
#include "ui/uiscreen.h"

// The slots UIScreen::InitWidgets builds, as this screen and the screens derived from it fill them.
enum MatchInfoWidget
{
    MATCHINFO_WIDGET_INFO   = UI_WIDGET_TAB_FIRST,
    MATCHINFO_WIDGET_AWARDS = UI_WIDGET_TAB_FIRST + 1,
    MATCHINFO_WIDGET_DONE   = UI_WIDGET_TAB_FIRST + 2,
    // Kiosk mode puts save and load in Done's slot and moves Done down one.
    MATCHINFO_WIDGET_SAVE_LOAD  = UI_WIDGET_TAB_FIRST + 2,
    MATCHINFO_WIDGET_KIOSK_DONE = UI_WIDGET_TAB_FIRST + 3
};

// The constructor calls UIScreen's at 0x0044d540.
// VTABLE: HYPERX 0x004b2350
class MatchInfoUIScreen : public UIScreen
{
public:
    void OnFirstTabClick() override;
    void OnSecondTabClick() override;
    void OnThirdTabClick() override;
    void OnExitTabClick() override;
    MatchInfoUIScreen();

    // Inline, so the derived screens that declare none share this body.
    // FUNCTION: HYPERX 0x004581c0
    ~MatchInfoUIScreen() override = default;

    void RegisterBitmaps();
    void UnloadBitmaps();

    int m_nNextState; // 0x474
};

// SYNTHETIC: HYPERX 0x0041bb90
// MatchInfoUIScreen::`scalar deleting destructor'

#endif
