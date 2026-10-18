#include "common/compat.h"
#include "input/inputevent.h"
#include "globals.h"
#include "ui/screen.h"
#include "ui/screens/lobbyscreen.h"
#include "ui/widget.h"
#include "ui/flipbookwidget.h"
#include "ui/loopflipbookwidget.h"
#include "ui/signflipbookwidget.h"
#include "graphics/gdi.h"
#include "engine/game.h"
#include "sound/soundids.h"

// FUNCTION: HYPERX 0x0048ced0
LobbyScreen::~LobbyScreen()
{
}

// Six widgets and twenty-six bitmaps; only the bitmap that sizes a widget is kept.
// FUNCTION: HYPERX 0x0048cf00
LobbyScreen::LobbyScreen()
{
    Widget* pWidget;
    // STRING: HYPERX 0x004c87f4
    m_hBackground = GDI::LoadBitmapAsBackground("Lobby", &m_bPaletted);
    m_nLastWidget = LOBBY_PLAY_SIGN;

    // STRING: HYPERX 0x004c87ec
    m_hBitmap = GDI::LoadBitmapHandle("Dadoor1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new FlipbookWidget(483, 209, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 5, 0, 5, 5, this, SOUND_DOOROPN1, SOUND_DOORCLS1, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrame(m_hBitmap);
    // STRING: HYPERX 0x004c87e4
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Dadoor2"));
    // STRING: HYPERX 0x004c87dc
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Dadoor3"));
    // STRING: HYPERX 0x004c87d4
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Dadoor4"));
    // STRING: HYPERX 0x004c87cc
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Dadoor5"));
    // STRING: HYPERX 0x004c87c4
    m_hBitmap = GDI::LoadBitmapHandle("Pldoor1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget =
        AppendWidget(new FlipbookWidget(290, 255, m_bitmap.nWidth + 40, m_bitmap.nHeight + 40, 1, 0, 5, 0, 5, 5, this, SOUND_DOOROPN2, SOUND_DOORCLS2, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 20, 20, pWidget->GetPositionX() + 20, pWidget->GetPositionY() + 20);
    pWidget->AppendFrameWithPos(m_hBitmap, m_bitmap.nWidth, m_bitmap.nHeight, 20, 20, 0, 0);
    // STRING: HYPERX 0x004c87bc
    pWidget->AppendFrameWithPos(GDI::LoadBitmapHandle("Pldoor2"), m_bitmap.nWidth, m_bitmap.nHeight, 20, 20, 0, 0);
    // STRING: HYPERX 0x004c87b4
    pWidget->AppendFrameWithPos(GDI::LoadBitmapHandle("Pldoor3"), m_bitmap.nWidth, m_bitmap.nHeight, 20, 20, 0, 0);
    // STRING: HYPERX 0x004c87ac
    pWidget->AppendFrameWithPos(GDI::LoadBitmapHandle("Pldoor4"), m_bitmap.nWidth, m_bitmap.nHeight, 20, 20, 0, 0);
    // STRING: HYPERX 0x004c87a4
    pWidget->AppendFrameWithPos(GDI::LoadBitmapHandle("Pldoor5"), m_bitmap.nWidth, m_bitmap.nHeight, 20, 20, 0, 0);
    // STRING: HYPERX 0x004c879c
    m_hBitmap = GDI::LoadBitmapHandle("Opdoor1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new FlipbookWidget(0, 209, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 5, 0, 5, 5, this, SOUND_DOOROPN1, SOUND_DOORCLS1, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrame(m_hBitmap);
    // STRING: HYPERX 0x004c8794
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Opdoor2"));
    // STRING: HYPERX 0x004c878c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Opdoor3"));
    // STRING: HYPERX 0x004c8784
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Opdoor4"));
    // STRING: HYPERX 0x004c877c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Opdoor5"));
    // STRING: HYPERX 0x004c8774
    m_hBitmap = GDI::LoadBitmapHandle("ltlite1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new LoopFlipbookWidget(254, 302, m_bitmap.nWidth, m_bitmap.nHeight, 1, 1, 10, 0, 10, 10, this, 1, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrame(m_hBitmap);
    // STRING: HYPERX 0x004c876c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("ltlite2"));
    // STRING: HYPERX 0x004c8764
    pWidget->AppendFrame(GDI::LoadBitmapHandle("ltlite3"));
    // STRING: HYPERX 0x004c875c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("ltlite4"));
    // STRING: HYPERX 0x004c8754
    pWidget->AppendFrame(GDI::LoadBitmapHandle("ltlite5"));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    // STRING: HYPERX 0x004c874c
    m_hBitmap = GDI::LoadBitmapHandle("rtlite1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new LoopFlipbookWidget(358, 302, m_bitmap.nWidth, m_bitmap.nHeight, 1, 1, 10, 0, 10, 10, this, 1, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrame(m_hBitmap);
    // STRING: HYPERX 0x004c8744
    pWidget->AppendFrame(GDI::LoadBitmapHandle("rtlite2"));
    // STRING: HYPERX 0x004c873c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("rtlite3"));
    // STRING: HYPERX 0x004c8734
    pWidget->AppendFrame(GDI::LoadBitmapHandle("rtlite4"));
    // STRING: HYPERX 0x004c872c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("rtlite5"));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    // STRING: HYPERX 0x004c8724
    m_hBitmap = GDI::LoadBitmapHandle("Plsign");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new SignFlipbookWidget(
        251, 170, m_bitmap.nWidth, m_bitmap.nHeight + 80, 1, 0, 1, 0, 1, 1, this, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrameWithPos(m_hBitmap, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 0, 0);

    InitAllWidgets();
}

// The lights are in the table and empty.
// FUNCTION: HYPERX 0x0048d820
void LobbyScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case LOBBY_DATA_DOOR:
            g_nSetupReturnState = ESTATE_LOBBY;

            if (nMsg == INPUT_LEFT_UP)
            {
                Game::SetEngineState(ESTATE_MATCH_INFO);
            }
            break;

        case LOBBY_PLAY_DOOR:
            if (nMsg == INPUT_MOUSE_MOVE)
            {
                m_apWidgets[LOBBY_PLAY_SIGN]->SetHighlighted(1);
            }
            else if (nMsg == INPUT_LEFT_UP)
            {
                Game::SetEngineState(ESTATE_MAIN_MENU);
            }
            break;

        case LOBBY_OPTIONS_DOOR:
            g_nSetupReturnState = ESTATE_LOBBY;

            if (nMsg == INPUT_LEFT_UP)
            {
                Game::SetEngineState(ESTATE_GAME_CONFIG);
            }
            break;

        case LOBBY_LEFT_LIGHT:
        case LOBBY_RIGHT_LIGHT:
            break;

        case LOBBY_PLAY_SIGN:
            if (nMsg == INPUT_MOUSE_MOVE)
            {
                m_apWidgets[LOBBY_PLAY_DOOR]->SetHighlighted(1);
            }
            else if (nMsg == INPUT_LEFT_UP)
            {
                Game::SetEngineState(ESTATE_MAIN_MENU);
            }
            break;
    }
}
