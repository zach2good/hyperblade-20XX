#include "common/compat.h"
#include "input/inputevent.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"
#include "ui/uiglobals.h"
#include "ui/screens/gameintroscreen.h"
#include "graphics/gdi.h"
#include "engine/game.h"

// FUNCTION: HYPERX 0x004490f0
GameIntroScreen::GameIntroScreen()
{
    m_hBackground = GDI::LoadBitmapAsBackground(g_pszIntroBackground, &m_bPaletted);
}

// FUNCTION: HYPERX 0x00449150
void GameIntroScreen::OnWidgetMessage(int nMsg, unsigned int nWidget)
{
    if (nWidget >= kWidgetIDMouse && nWidget <= kWidgetIDReturn)
    {
        if (nMsg == kInputKeyUp || nMsg == kInputKeyDown || (m_bPointerOnWidget == 0 && nMsg == kInputLeftUp))
        {
            Game::SetEngineState(kEstateDromeExterior);
        }
    }
}

// FUNCTION: HYPERX 0x004491b0
GameIntroScreen::~GameIntroScreen() = default;

// STRING: HYPERX 0x004c282c
const char* g_pszIntroBackground = "gintro";
