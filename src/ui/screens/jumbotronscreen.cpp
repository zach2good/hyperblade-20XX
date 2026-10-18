#include "common/compat.h"
#include "input/inputevent.h"
#include "engine/gameengine.h"
#include "engine/teamblock.h"
#include "globals.h"
#include "ui/screen.h"
#include "graphics/gdi.h"
class JumbotronScreen;
#include "ui/pausemenu.h"
#include "ui/jumbotextlist.h"
#include "ui/widget.h"
#include "engine/teamsblock.h"
#include "ui/torchbitmaps.h"
#include <cstdio>
#include "ui/screens/jumbotronscreen.h"
#include "app/gameapp.h"
#include "net/gamesession.h"
#include "ui/flipbookwidget.h"
#include "ui/loopflipbookwidget.h"
#include "ui/displaywidget.h"
#include "engine/game.h"
#include "net/network.h"

// FUNCTION: HYPERX 0x0048e710
JumbotronScreen::~JumbotronScreen()
{
}

// The advert comes from jumbo.txt. Separators two and three show only for three or four periods.
// FUNCTION: HYPERX 0x0048e740
JumbotronScreen::JumbotronScreen()
{
    char        szHomeScore[20];
    char        szAwayScore[20];
    char        szAdvert[80];
    Widget*     pWidget;
    const char* pszName;

    JumboTextList list;

    // STRING: HYPERX 0x004c894c
    list.Load("jumbo.txt");

    // STRING: HYPERX 0x004c8944
    m_hBackground = GDI::LoadBitmapAsBackground("Jumbo", &m_bPaletted);
    m_nLastWidget = 10;

    // STRING: HYPERX 0x004c893c
    m_hBitmap = GDI::LoadBitmapHandle("Perlite");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);

    pWidget = AppendWidget(new LoopFlipbookWidget(0x14a, 0xa0, m_bitmap.nWidth, m_bitmap.nHeight, 0, 1, 1, 0, 1, 1, this, 0, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrame(m_hBitmap);

    pWidget = AppendWidget(new LoopFlipbookWidget(0x14a, 0xe1, m_bitmap.nWidth, m_bitmap.nHeight, 0, 1, 1, 0, 1, 1, this, 0, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());

    if (g_pGameEngine->m_clock.nPeriod == 3 || g_pGameEngine->m_clock.nPeriod == 4)
    {
        pWidget->AppendFrame(m_hBitmap);
    }
    else
    {
        pWidget->AppendFrame(nullptr);
    }

    pWidget = AppendWidget(new LoopFlipbookWidget(0x14a, 0x122, m_bitmap.nWidth, m_bitmap.nHeight, 0, 1, 1, 0, 1, 1, this, 0, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());

    if (g_pGameEngine->m_clock.nPeriod == 4)
    {
        pWidget->AppendFrame(m_hBitmap);
    }
    else
    {
        pWidget->AppendFrame(nullptr);
    }

    short       nRound = (short)g_pGameEngine->m_nTeamHomeIndex;
    TeamsBlock* pList  = g_pGameEngine->m_pTeamTable;

    if (nRound < pList->GetTeamCount())
    {
        pszName = pList->m_ppTeams[nRound]->szTsLogo;
    }
    else
    {
        pszName = nullptr;
    }

    m_hBitmap = GDI::LoadBitmapHandle(pszName);
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new DisplayWidget(0x82, 0x14, 0, 0, this, -1));
    pWidget->AppendFrame(m_hBitmap);

    nRound = (short)g_pGameEngine->m_nTeamAwayIndex;
    pList  = g_pGameEngine->m_pTeamTable;

    if (nRound < pList->GetTeamCount())
    {
        pszName = pList->m_ppTeams[nRound]->szTsLogo;
    }
    else
    {
        pszName = nullptr;
    }

    m_hBitmap = GDI::LoadBitmapHandle(pszName);
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new DisplayWidget(0x81, 0xf4, 0, 0, this, -1));
    pWidget->AppendFrame(m_hBitmap);

    m_hBitmap = GDI::LoadBitmapHandle("Perlite");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    AppendWidget(new DisplayWidget(0x82, 0x7d, 0, 0, this, -1));

    m_hBitmap = GDI::LoadBitmapHandle("Perlite");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    AppendWidget(new DisplayWidget(0x81, 0x160, 0, 0, this, -1));

    pWidget = AppendWidget(new DisplayWidget(0xe1, 0x9b, 0, 0, this, -1));
    // STRING: HYPERX 0x004bb814
    sprintf(szHomeScore, "%d", g_pGameEngine->GetHomeScore());
    pWidget->SetTextFlag(1);
    pWidget->SetText(szHomeScore);

    pWidget = AppendWidget(new DisplayWidget(0xe1, 0x181, 0, 0, this, -1));
    sprintf(szAwayScore, "%d", g_pGameEngine->GetAwayScore());
    pWidget->SetTextFlag(1);
    pWidget->SetText(szAwayScore);

    // STRING: HYPERX 0x004c8934
    m_hBitmap = GDI::LoadBitmapHandle("jumdata");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new FlipbookWidget(0x177, 0x16e, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(m_hBitmap);
    // STRING: HYPERX 0x004c8928
    pWidget->AppendFrame(GDI::LoadBitmapHandle("jumdatar"));
    // STRING: HYPERX 0x004c891c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("jumdatad"));
    pWidget->InitAfterLoad();

    list.GetRandom(szAdvert, sizeof(szAdvert));

    m_hBitmap = GDI::LoadBitmapHandle(szAdvert);
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new DisplayWidget(0x179, 0x42, 0, 0, this, -1));
    pWidget->AppendFrame(m_hBitmap);

    InitAllWidgets();
}

// A key-down acts at once; a mouse-up waits for the screen's own guard.

#define WIDGET_BACK          9
#define MATCH_STATE_FINISHED 4

// FUNCTION: HYPERX 0x0048ee40
void JumbotronScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    if (nId != WIDGET_BACK)
    {
        if (nId <= WIDGET_ID_MOUSE - 1)
        {
            return;
        }

        if (nId > WIDGET_ID_RETURN)
        {
            return;
        }

        if (nMsg != INPUT_KEY_UP && nMsg != INPUT_KEY_DOWN)
        {
            if (m_bPointerOnWidget != 0)
            {
                return;
            }

            if (nMsg != INPUT_LEFT_UP)
            {
                return;
            }
        }

        if (g_pGameEngine->m_clock.nPeriod == MATCH_STATE_FINISHED)
        {
            if (g_bSessionActive != 0)
            {
                GameSession::Disconnect();
            }

            if (g_nPlayerMode == PLAYER_MODE_EXHIBITION)
            {
                Game::SetEngineState(ESTATE_LOBBY);
            }
            else if (g_nPlayerMode == PLAYER_MODE_TOURNAMENT)
            {
                Game::SetEngineState(ESTATE_CHAMPIONSHIP_SETUP);
            }

            g_pGameEngine->ResetPeriodToFirst();
        }
        else
        {
            Game::SetEngineState(ESTATE_MATCH_START);
        }

        Screen::ClearBlack();

        InitFrame();

        if (g_bSessionActive != 0 && Network::ShowWaitingDialog() == 0)
        {
            g_gameApp.OnShutdown();
        }

        return;
    }

    if (nMsg != INPUT_LEFT_UP)
    {
        return;
    }

    ShowMatchInfo();
}
