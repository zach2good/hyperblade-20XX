#ifndef HYPERBLADE_MAINMENUSCREEN_H
#define HYPERBLADE_MAINMENUSCREEN_H

#include "common/compat.h"
#include "common/staticassert.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"

enum MainMenuWidget
{
    MAIN_MENU_ARENA_DOOR,
    // Turns between the two pages and shows the buttons when it stops.
    MAIN_MENU_SIGN,
    MAIN_MENU_BUTTON_FIRST,
    MAIN_MENU_BUTTON_COUNT = 5
};

// What g_nMainMenuPage holds.
enum MainMenuPage
{
    MAIN_MENU_PAGE_TOP,
    MAIN_MENU_PAGE_SINGLE
};

// The buttons on the top page.
enum MainMenuTopButton
{
    MAIN_MENU_SINGLE = MAIN_MENU_BUTTON_FIRST,
    MAIN_MENU_MULTI,
    MAIN_MENU_LOAD_GAME,
    // Empty on both pages.
    MAIN_MENU_GAP,
    MAIN_MENU_TO_LOBBY
};

// The buttons on the page Singl opens; the two between have no bitmaps.
enum MainMenuSingleButton
{
    MAIN_MENU_EXHIBITION = MAIN_MENU_BUTTON_FIRST,
    MAIN_MENU_TOURNAMENT,
    MAIN_MENU_PREVIOUS = MAIN_MENU_BUTTON_FIRST + 4
};

// What g_nMainMenuMatchChoice holds once a match type is picked.
enum MainMenuMatchChoice
{
    MAIN_MENU_MATCH_EXHIBITION = 4,
    MAIN_MENU_MATCH_TOURNAMENT = 5
};

// VTABLE: HYPERX 0x004b4058
class MainMenuScreen : public Screen
{
public:
    MainMenuScreen();
    ~MainMenuScreen();
    void         LoadButtonBitmaps();
    virtual void OnWidgetMessage(int nMsg, unsigned int nId);
};

STATIC_ASSERT(sizeof(MainMenuScreen) == 0x464);

// SYNTHETIC: HYPERX 0x0048d900
// MainMenuScreen::`scalar deleting destructor'

#endif
