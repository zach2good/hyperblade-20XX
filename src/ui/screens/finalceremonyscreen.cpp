#include "common/compat.h"
#include "input/inputevent.h"
#include "engine/teamblock.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"
#include "ui/pausemenu.h"
#include "engine/gameengine.h"
#include "engine/teamsblock.h"
#include <cstdio>
#include "ui/screens/finalceremonyscreen.h"
#include "app/gameapp.h"
#include "net/gamesession.h"
#include "ui/displaywidget.h"
#include "ui/loopflipbookwidget.h"
#include "graphics/gdi.h"
#include "engine/game.h"

// The flipbook frames are "finan01" to "finan60"; the prefix is re-read every iteration.
// FUNCTION: HYPERX 0x00442fc0
FinalCeremonyScreen::FinalCeremonyScreen()
{
    char        szFrameName[80];
    const char* pszName;

    m_nLastWidget = FINAL_CEREMONY_ANIMATION;

    m_hBackground = GDI::LoadBitmapAsBackground(g_pszFinaleBackground, &m_bPaletted);

    const short       nIndex = g_pGameEngine->m_nTeamHomeIndex;
    TeamsBlock* const pList  = g_pGameEngine->m_pTeamTable;

    if (nIndex < pList->GetTeamCount())
    {
        pszName = pList->m_ppTeams[nIndex]->szTsLogo;
    }
    else
    {
        pszName = nullptr;
    }

    GDI::Bitmap* hBitmap = GDI::LoadBitmapHandle(pszName);
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    Widget* const pWidget = AppendWidget(new DisplayWidget(0x2d, 0x4f, 0, 0, this, -1));
    pWidget->AppendFrame(hBitmap);

    // STRING: HYPERX 0x004c1f30
    hBitmap = GDI::LoadBitmapHandle("finan01");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    GDI::SafeDeleteBitmap(hBitmap);
    AppendWidget(new LoopFlipbookWidget(0x12c, 0, m_bitmap.nWidth, m_bitmap.nHeight, 1, 1, 0x3b, 0, 0x3b, 0x3b, this, 1, -1));

    for (int i = 0; i < 60; i++)
    {
        // STRING: HYPERX 0x004bc1a4
        sprintf(szFrameName, "%s%02d", g_pszFinaleAnimPrefix, i + 1);
        m_apWidgets[FINAL_CEREMONY_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle(szFrameName));
    }

    InitAllWidgets();
}

// FUNCTION: HYPERX 0x00443190
FinalCeremonyScreen::~FinalCeremonyScreen() = default;

// The id is compared unsigned.
// FUNCTION: HYPERX 0x004431a0
void FinalCeremonyScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    if (nId >= WIDGET_ID_MOUSE && nId <= WIDGET_ID_RETURN && (nMsg == INPUT_KEY_UP || nMsg == INPUT_KEY_DOWN || (m_bPointerOnWidget == 0 && nMsg == INPUT_LEFT_UP)))
    {
        g_pGameEngine->m_bMatchOver = 1;

        g_pGameEngine->ResetPeriodToFirst();

        if (g_bSessionActive != 0)
        {
            GameSession::Disconnect();
        }

        GameApp::ResetToSinglePlayer();
        Game::SetEngineState(ESTATE_TRAINING_END);
    }
}

// STRING: HYPERX 0x004c1f28
const char* g_pszFinaleBackground = "finext";

// STRING: HYPERX 0x004c1f20
const char* g_pszFinaleAnimPrefix = "finan";
