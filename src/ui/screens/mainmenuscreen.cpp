#include "common/compat.h"
#include "input/inputevent.h"
#include "app/gameapp.h"
#include "globals.h"
#include "ui/screen.h"
#include "ui/buttonwidget.h"
#include "ui/pausemenu.h"
#include "ui/widget.h"
#include "ui/torchbitmaps.h"
#include "ui/screens/mainmenuscreen.h"
#include "ui/saveobject.h"
#include "ui/flipbookwidget.h"
#include "ui/loopflipbookwidget.h"
#include "graphics/gdi.h"
#include "engine/game.h"
#include "engine/gameengine.h"
#include "common/stub.h"
#include "sound/soundids.h"

// FUNCTION: HYPERX 0x0048d8f0
MainMenuScreen::~MainMenuScreen() = default;

// FUNCTION: HYPERX 0x0048d920
MainMenuScreen::MainMenuScreen()
{
    Widget* pWidget;
    // STRING: HYPERX 0x004c888c
    m_hBackground = GDI::LoadBitmapAsBackground("Lounge", &m_bPaletted);
    m_nLastWidget = MAIN_MENU_TO_LOBBY;

    // STRING: HYPERX 0x004c8884
    m_hBitmap = GDI::LoadBitmapHandle("Ardoor1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(std::make_unique<FlipbookWidget>(370, 214, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 4, 0, 4, 4, this, kSoundDooropn2, kSoundDoorcls2, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrame(m_hBitmap);
    // STRING: HYPERX 0x004c887c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Ardoor2"));
    // STRING: HYPERX 0x004c8874
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Ardoor3"));
    // STRING: HYPERX 0x004c886c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Ardoor4"));
    // STRING: HYPERX 0x004c8864
    m_hBitmap = GDI::LoadBitmapHandle("gs01");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(std::make_unique<LoopFlipbookWidget>(45, 177, m_bitmap.nWidth, m_bitmap.nHeight, 0, 1, 11, 0, 11, 11, this, 0, -1));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AppendFrame(m_hBitmap);
    // STRING: HYPERX 0x004c885c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs02"));
    // STRING: HYPERX 0x004c8854
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs03"));
    // STRING: HYPERX 0x004c884c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs04"));
    // STRING: HYPERX 0x004c8844
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs05"));
    // STRING: HYPERX 0x004c883c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs06"));
    // STRING: HYPERX 0x004c8834
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs07"));
    // STRING: HYPERX 0x004c882c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs08"));
    // STRING: HYPERX 0x004c8824
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs09"));
    // STRING: HYPERX 0x004c881c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("gs10"));
    pWidget->AppendFrameWithPos(m_hBackground, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    // STRING: HYPERX 0x004c8814
    m_hBitmap = GDI::LoadBitmapHandle("Singl");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(std::make_unique<ButtonWidget>(0, 96, 208, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, kSoundSxbutt5));
    GDI::SafeDeleteBitmap(m_hBitmap);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->SetHidden(1);

    // STRING: HYPERX 0x004c880c
    m_hBitmap = GDI::LoadBitmapHandle("Multi");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(std::make_unique<ButtonWidget>(0, 96, 235, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, kSoundSxbutt5));
    GDI::SafeDeleteBitmap(m_hBitmap);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->SetHidden(1);

    // STRING: HYPERX 0x004c8804
    m_hBitmap = GDI::LoadBitmapHandle("Loadgm");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(std::make_unique<ButtonWidget>(0, 96, 262, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, kSoundSxbutt5));
    GDI::SafeDeleteBitmap(m_hBitmap);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->SetHidden(1);

    pWidget = AppendWidget(std::make_unique<FlipbookWidget>(96, 289, 0, 0, 0, 0, 2, 0, 1, 2, this, kSoundSxbutt5, -1, -1));
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->SetHidden(1);

    // STRING: HYPERX 0x004c87fc
    m_hBitmap = GDI::LoadBitmapHandle("tolobb");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(std::make_unique<FlipbookWidget>(96, 315, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1, -1, kSoundSxbutt5));
    GDI::SafeDeleteBitmap(m_hBitmap);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->SetHidden(1);

    InitAllWidgets();
    LoadButtonBitmaps();
}

// Each case names its slot by the top page's button; the inner switch picks the page.
// FUNCTION: HYPERX 0x0048df90
void MainMenuScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case MAIN_MENU_ARENA_DOOR:
            if (nMsg != kInputLeftUp)
            {
                break;
            }

            if (g_nMainMenuMatchChoice == MAIN_MENU_MATCH_TOURNAMENT)
            {
                g_nPlayerMode = kPlayerModeTournament;
            }
            else if (g_nMainMenuMatchChoice == MAIN_MENU_MATCH_EXHIBITION)
            {
                g_nPlayerMode = kPlayerModeExhibition;
            }

            if (g_nPlayerMode == kPlayerModeTournament && g_pGameEngine->m_pSaveObject->GetBracketPos() == 0)
            {
                Game::SetEngineState(kEstateGameIntro);
            }
            else
            {
                Game::SetEngineState(kEstateDromeExterior);
            }

            FlushBitmapCache();
            break;

        case MAIN_MENU_SIGN:
            if (nMsg == kWidgetMsgLoopDone)
            {
                m_apWidgets[MAIN_MENU_BUTTON_FIRST]->SetDirtyFlag(1);
                m_apWidgets[MAIN_MENU_BUTTON_FIRST + 1]->SetDirtyFlag(1);
                m_apWidgets[MAIN_MENU_BUTTON_FIRST + 2]->SetDirtyFlag(1);
                m_apWidgets[MAIN_MENU_BUTTON_FIRST + 3]->SetDirtyFlag(1);
                m_apWidgets[MAIN_MENU_BUTTON_FIRST + 4]->SetDirtyFlag(1);

                m_apWidgets[MAIN_MENU_BUTTON_FIRST]->SetHidden(0);
                m_apWidgets[MAIN_MENU_BUTTON_FIRST + 1]->SetHidden(0);
                m_apWidgets[MAIN_MENU_BUTTON_FIRST + 2]->SetHidden(0);
                m_apWidgets[MAIN_MENU_BUTTON_FIRST + 3]->SetHidden(0);
                m_apWidgets[MAIN_MENU_BUTTON_FIRST + 4]->SetHidden(0);
            }
            break;

        case MAIN_MENU_SINGLE:
            switch (g_nMainMenuPage)
            {
                case MAIN_MENU_PAGE_TOP:
                    if (nMsg != kInputLeftUp)
                    {
                        break;
                    }

                    g_nMainMenuPage      = MAIN_MENU_PAGE_SINGLE;
                    g_nMainMenuTopChoice = 1;

                    m_apWidgets[MAIN_MENU_SIGN]->SetCurrentFrame(0);
                    m_apWidgets[MAIN_MENU_SIGN]->SetTargetFrame(0xb);
                    m_apWidgets[MAIN_MENU_SIGN]->SetDepressed(1);

                    LoadButtonBitmaps();

                    m_apWidgets[MAIN_MENU_BUTTON_FIRST]->SetHidden(1);
                    m_apWidgets[MAIN_MENU_BUTTON_FIRST + 1]->SetHidden(1);
                    m_apWidgets[MAIN_MENU_BUTTON_FIRST + 2]->SetHidden(1);
                    m_apWidgets[MAIN_MENU_BUTTON_FIRST + 3]->SetHidden(1);
                    m_apWidgets[MAIN_MENU_BUTTON_FIRST + 4]->SetHidden(1);
                    break;

                case MAIN_MENU_PAGE_SINGLE:
                    if (nMsg != kInputLeftUp)
                    {
                        break;
                    }

                    g_nMainMenuMatchChoice = MAIN_MENU_MATCH_EXHIBITION;
                    g_nPlayerMode          = kPlayerModeExhibition;
                    g_nSetupReturnState    = kEstateMainMenu;

                    Game::SetEngineState(kEstateHeadToHeadSetup);
                    break;

                default:
                    RELEASE_ASSERT(0);
                    break;
            }
            break;

        case MAIN_MENU_MULTI:
            switch (g_nMainMenuPage)
            {
                case MAIN_MENU_PAGE_TOP:
                    if (nMsg != kInputLeftUp)
                    {
                        break;
                    }

                    // The DirectPlay set-up dialogs went with the resources; see the README's TODO.
                    GameApp::ShowMessageBox("Network play is not available in this build yet.", g_gameApp.m_strCaption.m_pchData, Window::MessageButtons::OK);

                    InitFrame();
                    break;

                case MAIN_MENU_PAGE_SINGLE:
                    if (nMsg != kInputLeftUp)
                    {
                        break;
                    }

                    g_nMainMenuMatchChoice = MAIN_MENU_MATCH_TOURNAMENT;
                    g_nPlayerMode          = kPlayerModeTournament;
                    g_nSetupReturnState    = kEstateMainMenu;

                    Game::SetEngineState(kEstateTournamentSetup);
                    break;

                default:
                    RELEASE_ASSERT(0);
                    break;
            }
            break;

        case MAIN_MENU_LOAD_GAME:
            switch (g_nMainMenuPage)
            {
                case MAIN_MENU_PAGE_TOP:
                    if (nMsg == kInputLeftUp)
                    {
                        Game::SetEngineState(kEstateSaveLoad);
                    }
                    break;

                case MAIN_MENU_PAGE_SINGLE:
                    break;

                default:
                    RELEASE_ASSERT(0);
                    break;
            }
            break;

        case MAIN_MENU_TO_LOBBY:
            switch (g_nMainMenuPage)
            {
                case MAIN_MENU_PAGE_TOP:
                    if (nMsg == kInputLeftUp)
                    {
                        Game::SetEngineState(kEstateLobby);
                    }
                    break;

                case MAIN_MENU_PAGE_SINGLE:
                    if (nMsg != kInputLeftUp)
                    {
                        break;
                    }

                    g_nMainMenuPage = MAIN_MENU_PAGE_TOP;

                    m_apWidgets[MAIN_MENU_SIGN]->SetCurrentFrame(0xb);
                    m_apWidgets[MAIN_MENU_SIGN]->SetTargetFrame(0);
                    m_apWidgets[MAIN_MENU_SIGN]->SetDepressed(1);

                    LoadButtonBitmaps();

                    m_apWidgets[MAIN_MENU_BUTTON_FIRST]->SetHidden(1);
                    m_apWidgets[MAIN_MENU_BUTTON_FIRST + 1]->SetHidden(1);
                    m_apWidgets[MAIN_MENU_BUTTON_FIRST + 2]->SetHidden(1);
                    m_apWidgets[MAIN_MENU_BUTTON_FIRST + 3]->SetHidden(1);
                    m_apWidgets[MAIN_MENU_BUTTON_FIRST + 4]->SetHidden(1);
                    break;

                default:
                    RELEASE_ASSERT(0);
                    break;
            }
            break;

        case MAIN_MENU_GAP:
        case kWidgetIDMouse:
        case kWidgetIDReturn:
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}

// Plain, "r" rollover and "d" pressed. Empty buttons keep three frames so indices line up.
// FUNCTION: HYPERX 0x0048e350
void MainMenuScreen::LoadButtonBitmaps()
{
    for (int i = 0; i < MAIN_MENU_BUTTON_COUNT; i++)
    {
        m_apWidgets[i + MAIN_MENU_BUTTON_FIRST]->ClearFrames();
    }

    switch (g_nMainMenuPage)
    {
        case MAIN_MENU_PAGE_SINGLE:
            // STRING: HYPERX 0x004c8914
            m_apWidgets[MAIN_MENU_EXHIBITION]->AppendFrame(GDI::LoadBitmapHandle("Exhib"));
            // STRING: HYPERX 0x004c890c
            m_apWidgets[MAIN_MENU_EXHIBITION]->AppendFrame(GDI::LoadBitmapHandle("Exhibr"));
            // STRING: HYPERX 0x004c8904
            m_apWidgets[MAIN_MENU_EXHIBITION]->AppendFrame(GDI::LoadBitmapHandle("Exhibd"));
            m_apWidgets[MAIN_MENU_EXHIBITION]->InitAfterLoad();

            // STRING: HYPERX 0x004c88fc
            m_apWidgets[MAIN_MENU_TOURNAMENT]->AppendFrame(GDI::LoadBitmapHandle("Tourn"));
            // STRING: HYPERX 0x004c88f4
            m_apWidgets[MAIN_MENU_TOURNAMENT]->AppendFrame(GDI::LoadBitmapHandle("Tournr"));
            // STRING: HYPERX 0x004c88ec
            m_apWidgets[MAIN_MENU_TOURNAMENT]->AppendFrame(GDI::LoadBitmapHandle("Tournd"));
            m_apWidgets[MAIN_MENU_TOURNAMENT]->InitAfterLoad();

            m_apWidgets[MAIN_MENU_BUTTON_FIRST + 2]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_BUTTON_FIRST + 2]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_BUTTON_FIRST + 2]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_BUTTON_FIRST + 2]->InitAfterLoad();

            m_apWidgets[MAIN_MENU_GAP]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_GAP]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_GAP]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_GAP]->InitAfterLoad();

            // STRING: HYPERX 0x004c88e4
            m_apWidgets[MAIN_MENU_PREVIOUS]->AppendFrame(GDI::LoadBitmapHandle("Prvius"));
            // STRING: HYPERX 0x004c88dc
            m_apWidgets[MAIN_MENU_PREVIOUS]->AppendFrame(GDI::LoadBitmapHandle("Prviusr"));
            // STRING: HYPERX 0x004c88d4
            m_apWidgets[MAIN_MENU_PREVIOUS]->AppendFrame(GDI::LoadBitmapHandle("Prviusd"));
            m_apWidgets[MAIN_MENU_PREVIOUS]->InitAfterLoad();

            switch (g_nMainMenuMatchChoice)
            {
                case 0:
                    break;

                case MAIN_MENU_MATCH_EXHIBITION:
                    break;

                case MAIN_MENU_MATCH_TOURNAMENT:
                    break;

                default:
                    RELEASE_ASSERT(0);
                    break;
            }

            break;

        case MAIN_MENU_PAGE_TOP:
            // STRING: HYPERX 0x004c8814
            m_apWidgets[MAIN_MENU_SINGLE]->AppendFrame(GDI::LoadBitmapHandle("Singl"));
            // STRING: HYPERX 0x004c88cc
            m_apWidgets[MAIN_MENU_SINGLE]->AppendFrame(GDI::LoadBitmapHandle("Singlr"));
            // STRING: HYPERX 0x004c88c4
            m_apWidgets[MAIN_MENU_SINGLE]->AppendFrame(GDI::LoadBitmapHandle("Singld"));
            m_apWidgets[MAIN_MENU_SINGLE]->InitAfterLoad();

            // STRING: HYPERX 0x004c880c
            m_apWidgets[MAIN_MENU_MULTI]->AppendFrame(GDI::LoadBitmapHandle("Multi"));
            // STRING: HYPERX 0x004c88bc
            m_apWidgets[MAIN_MENU_MULTI]->AppendFrame(GDI::LoadBitmapHandle("Multir"));
            // STRING: HYPERX 0x004c88b4
            m_apWidgets[MAIN_MENU_MULTI]->AppendFrame(GDI::LoadBitmapHandle("Multid"));
            m_apWidgets[MAIN_MENU_MULTI]->InitAfterLoad();

            // STRING: HYPERX 0x004c8804
            m_apWidgets[MAIN_MENU_LOAD_GAME]->AppendFrame(GDI::LoadBitmapHandle("Loadgm"));
            // STRING: HYPERX 0x004c88ac
            m_apWidgets[MAIN_MENU_LOAD_GAME]->AppendFrame(GDI::LoadBitmapHandle("Loadgmr"));
            // STRING: HYPERX 0x004c88a4
            m_apWidgets[MAIN_MENU_LOAD_GAME]->AppendFrame(GDI::LoadBitmapHandle("Loadgmd"));
            m_apWidgets[MAIN_MENU_LOAD_GAME]->InitAfterLoad();

            m_apWidgets[MAIN_MENU_GAP]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_GAP]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_GAP]->AppendFrame(nullptr);
            m_apWidgets[MAIN_MENU_GAP]->InitAfterLoad();

            // STRING: HYPERX 0x004c87fc
            m_apWidgets[MAIN_MENU_TO_LOBBY]->AppendFrame(GDI::LoadBitmapHandle("tolobb"));
            // STRING: HYPERX 0x004c889c
            m_apWidgets[MAIN_MENU_TO_LOBBY]->AppendFrame(GDI::LoadBitmapHandle("tolobbr"));
            // STRING: HYPERX 0x004c8894
            m_apWidgets[MAIN_MENU_TO_LOBBY]->AppendFrame(GDI::LoadBitmapHandle("tolobbd"));
            m_apWidgets[MAIN_MENU_TO_LOBBY]->InitAfterLoad();

            switch (g_nMainMenuTopChoice)
            {
                case 0:
                    break;

                case 1:
                    break;

                case 2:
                    break;

                default:
                    RELEASE_ASSERT(0);
                    break;
            }

            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}
