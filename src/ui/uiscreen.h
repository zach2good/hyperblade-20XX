#ifndef HYPERBLADE_UISCREEN_H
#define HYPERBLADE_UISCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "widget.h"
#include "ui/screen.h"

// The chrome InitWidgets builds for every screen derived from UIScreen. A plate's loop redraws
// the widgets in front of it.
enum UIScreenWidget
{
    UI_WIDGET_TAB_PLATE      = 0,
    UI_WIDGET_PAGE_TAB_PLATE = 1,
    UI_WIDGET_CONFIRM_PLATE  = 2,
    UI_WIDGET_STAT_PLATE     = 3,
    // The column of tabs on the left.
    UI_WIDGET_TAB_FIRST = 4,
    UI_WIDGET_TAB_COUNT = 4,
    // The row along the bottom, where each setup screen names its pages.
    UI_WIDGET_PAGE_TAB_FIRST = 8,
    UI_WIDGET_PAGE_TAB_COUNT = 6,
    UI_WIDGET_OKAY           = 14,
    UI_WIDGET_CANCEL         = 15,
    UI_WIDGET_LEFT_ARROW     = 16,
    UI_WIDGET_RIGHT_ARROW    = 17,
    UI_WIDGET_STAT_FIRST     = 18,
    UI_WIDGET_STAT_COUNT     = 4,
    // The full-width page above the chrome.
    UI_WIDGET_PAGE = 22,
    // A derived screen appends its own widgets from here.
    UI_WIDGET_OWN_FIRST = 23
};

// VTABLE: HYPERX 0x004b323c
class UIScreen : public Screen
{
public:
    void InitWidgets();

    // Four screens call this, only one a TournamentSetupUIScreen; it rewinds only the first own widget.
    void ResetFrameOnLoopDone(int nMsg);

    virtual void OnWidgetMessage(int nMsg, unsigned int nId);

    // Slots three, four and ten to twenty seven of 0x004b323c all hold Object::Trace, so each is
    // empty here.
    // FUNCTION: HYPERX100 0x0042e5a0
    virtual void RefreshSelection()
    {
    }

    // FUNCTION: HYPERX100 0x0042e5b0
    virtual void RefreshFrames()
    {
    }

    // Two arguments: symbols/vtablearity.h reads slot five as an eight byte pop.
    virtual void OnOtherButton(int nMsg, unsigned int nId);
    virtual void RefreshTabs();
    virtual void RefreshPageTabs();
    virtual void RefreshConfirmButtons();
    virtual void RefreshStatRow();

    // Named for the UIScreenWidget slot each answers.
    // FUNCTION: HYPERX100 0x00433ba0
    virtual void OnFirstTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x00433bb0
    virtual void OnSecondTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x00433bc0
    virtual void OnThirdTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x00433bd0
    virtual void OnExitTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e5c0
    virtual void OnFirstPageTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e5d0
    virtual void OnSecondPageTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e5e0
    virtual void OnThirdPageTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e5f0
    virtual void OnFourthPageTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e600
    virtual void OnFifthPageTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e610
    virtual void OnSixthPageTabClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e620
    virtual void OnOkayClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e630
    virtual void OnCancelClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e640
    virtual void OnLeftArrowClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e650
    virtual void OnRightArrowClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e660
    virtual void OnFirstStatClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e670
    virtual void OnSecondStatClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e680
    virtual void OnThirdStatClick()
    {
    }

    // FUNCTION: HYPERX100 0x0042e690
    virtual void OnFourthStatClick()
    {
    }

    UIScreen();
    ~UIScreen();

    void BuildLockedFlagsArray();
    void PreloadBitmaps();
#ifdef HYPERX_V100
    void UnloadBitmaps();
#endif

protected:
    int m_nSelectionMode; // 0x464
    // Non-zero where that team's panel bitmap is missing.
    int* m_pLockedFlags; // 0x468
    int  m_nLockedCount; // 0x46c
    int  m_nUnusedD;     // 0x470
};

// SYNTHETIC: HYPERX 0x0044d520
// UIScreen::`scalar deleting destructor'

#endif
