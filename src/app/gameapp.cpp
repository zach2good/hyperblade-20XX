#include "common/compat.h"
#include <cstdlib>
#include <string>
#include "graphics/hudlayer.h"
#include "graphics/softrenderer.h"
#include "engine/gameoptions.h"
#include "globals.h"
#include "graphics/graphicsglobals.h"
#include "engine/engineglobals.h"
#include "common/pathstring.h"
#include "appglobals.h"
#include "common/fileobj.h"
#include "common/commonglobals.h"
#include "engine/cameraobject.h"
#include "engine/gameengine.h"
#include "graphics/renderer.h"
#include "app/window.h"
#include "common/stringtable.h"
#include <SDL3/SDL.h>
#include "graphics/gdi.h"
#include "common/string.h"
struct GameApp;
#include "ui/torchbitmaps.h"
#include "ui/buttonwidget.h"
#include "ui/screens/dromeexteriorscreen.h"
class GameConfigUIScreen;
class KeyConfigScreen;
class JumbotronScreen;
#include "ui/screens/lobbyscreen.h"
class AwardsUIScreen;
#include "ui/pausemenu.h"
#include "ui/saveload.h"
#include "ui/screens/saveloaduiscreen.h"
#include "ui/widget.h"
#include "ui/screen.h"
#include "ui/screens/leaguesetupuiscreen.h"
#include "gameapp.h"
#include "ui/screens/gameconfiguiscreen.h"
#include "ui/screens/mainmenuscreen.h"
#include "ui/screens/tournamentsetupuiscreen.h"
#include "ui/screens/headtoheadsetupuiscreen.h"
#include "ui/screens/awardsuiscreen.h"
#include "ui/screens/keyconfigscreen.h"
#include "ui/screens/jumbotronscreen.h"
#include "ui/screens/finalceremonyscreen.h"
#include "input/inputglobals.h"
#include "sound/audiomgr.h"
#include "sound/soundmixer.h"
#include "engine/messagequeue.h"
#include "input/keyboarddeviceobj.h"
#include "engine/objectclasses.h"
#include "engine/teamblock.h"
#include "engine/devicector.h"
#include "net/gamesession.h"
#include <stdarg.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iterator>
#include "ui/screens/championshipsetupuiscreen.h"
#include "graphics/viewextent.h"
#include "ui/screens/pausemenufullscreen.h"
#include "ui/screens/pausemenuminimalscreen.h"
#include "ui/screens/pausemenunosavescreen.h"
#include "ui/screens/multisetupoptionsscreen.h"
#include "ui/screens/gameintroscreen.h"
#include "ui/screens/trainingendscreen.h"
#include "engine/engineobject.h"
#include "input/gamepad.h"
#include "graphics/colorpalette.h"
#include "config.h"
#include "frametimer.h"
#include <new.h>
#include "graphics/display.h"
#include "ui/infobarbase.h"
#include "graphics/cgpalette.h"
#include "input/input.h"
#include "app/win32.h"
#include "app/wndproc.h"
#include "app/menuids.h"
#include "net/network.h"
#include "engine/game.h"
#include "engine/framepacing.h"
#include "graphics/graphicssettings.h"
#include "app/stringids.h"
#include "common/staticinit.h"

float g_flUnusedConfigDialogA = g_flZeroConfigDialog;
float g_flUnusedConfigDialogB = g_flZeroConfigDialog;
float g_flUnusedConfigDialogC = g_flZeroConfigDialog;

// id_ACF_AnimationPeek in DATA/ENUMIDS.H. It opened a debug dialog, which went with the resources.
// FUNCTION: HYPERX 0x004013d0
void GameApp::AnimationPeek(Interactor*, PlanNode*, short*, int)
{
}

// FUNCTION: HYPERX 0x004017a0
void GameApp::Fatal(const char* pszFormat, ...)
{
    va_list argList;

    va_start(argList, pszFormat);

    FatalErrorFmt(pszFormat, argList);

    va_end(argList);
}

// FUNCTION: HYPERX 0x00411be0
void GameApp::Error(const char* pszFormat, ...)
{
    va_list argList;

    va_start(argList, pszFormat);

    ErrorFmt(pszFormat, argList);

    va_end(argList);
}

// FUNCTION: HYPERX 0x00418c10
void GameApp::SetModelPath(int bTexEnabled)
{
    if (bTexEnabled != 0)
    {
        // STRING: HYPERX 0x004bba8c
        g_gameApp.m_strModelPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "Models\\Mdltxt"));
    }
    else
    {
        // STRING: HYPERX 0x004bba7c
        g_gameApp.m_strModelPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "Models\\Mdlshd"));
    }
}

static std::string s_arguments;

const char* GameApp::Arguments()
{
    return s_arguments.c_str();
}

// FUNCTION: HYPERX 0x00418e10
HWND GameApp::Initialize(const char* pszArguments)
{
    s_arguments = pszArguments;

    const char* pszAppClassName;

    // An unaware process is DPI-virtualised, and the window would be drawn stretched.
    SetProcessDPIAware();

    _set_new_handler(GameApp::OutOfMemoryHandler);
    // The text comes from the player's own HYPERX.EXE the first time, and from the strings file after.
    const char* const pszBase = SDL_GetBasePath();

    if (pszBase == 0 || !StringTable::Load(pszBase))
    {
        Window::ShowMessage("HyperBlade 20XX runs from your HyperBlade folder. Put hyperblade_20XX.exe beside HYPERX.EXE and start it from there.", "HyperBlade 20XX", Window::MessageButtons::OK, Window::MessageIcon::STOP);

        std::_Exit(1);
    }

    GameApp::InitializeResourcePaths();
    GameApp::ParseCommandLine(pszArguments);

    // A modern desktop keeps its own mode, so the game runs windowed.
    g_bFullScreen = 0;

    g_pColorPalette = new ColorPalette;

    pszAppClassName = g_gameApp.m_strCaption.m_pchData;
    const HWND hWnd = GameApp::CreateMainWindow(pszAppClassName);

    g_gameApp.SetWindowAndCallbacks(hWnd, GameApp::OnStartup, GameApp::OnExit);

    g_gameApp.m_bCursorWasVisible = Display::Initialize();

    Window::Show();
    GameApp::SetCursorVisible(1);

    g_nSplashFrame = 0;
    GameApp::NextSplash();

    // Windows shows its start-up busy cursor until a new program first takes its messages.
    Window::Pump();

    Network::ValidateStaticObjects();

    // STRING: HYPERX 0x004bba9c
    Config::LoadPreferences("Prefs.txt");

    Game::InitializeEngine();

    GameApp::SetupViewports();

    Graphics::Initialize(1);

    Window::ApplyCurrentSize();

    g_frameTimer.ProcessInitialFrameUpdate();

    if (g_bNoSoundArg != 0)
    {
        g_pAudioMgr->wNoSound = 1;
    }

    return hWnd;
}

// FUNCTION: HYPERX 0x00419070
void GameApp::InitializeResourcePaths()
{
    char* const pszClassName = g_gameApp.m_stringPool.LoadText(STRING_HYPERBLADE);

    g_gameApp.m_strCaption.Assign(String::SafeStrlen(pszClassName), pszClassName);

    PathString strDirectory(MAX_PATH);

    // SDL's base path ends in a separator.
    const char* const pszBase = SDL_GetBasePath();

    if (pszBase != 0)
    {
        strDirectory.m_str.Assign(String::SafeStrlen(pszBase), pszBase);
    }

    // STRING: HYPERX 0x004bbad8
    g_gameApp.m_strExeDir.m_str.AssignFromString(String::Concat(strDirectory.m_str, "Data\\"));

    // STRING: HYPERX 0x004bbad0
    g_gameApp.m_strAssetsPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "assets8"));

    // STRING: HYPERX 0x004bbac4
    g_gameApp.m_strAnimDataPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "AnimData"));

    GameApp::SetModelPath(0);

    // STRING: HYPERX 0x004bbabc
    g_gameApp.m_strPlansPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "Plans"));

    // STRING: HYPERX 0x004bbab4
    g_gameApp.m_strSoundsPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "Sounds"));

    // STRING: HYPERX 0x004bbaa8
    g_gameApp.m_strTexturesPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "Textures"));
}

constexpr auto SPLASH_MS = 3000ul;

static unsigned int s_dwSplashShown = 0;

// FUNCTION: HYPERX 0x00419290
void GameApp::NextSplash()
{
    static const constexpr char* s_apszSplashes[] = {
        // STRING: HYPERX 0x004bbaf4
        "SPLASHWZ",
        // STRING: HYPERX 0x004bbae8
        "SPLASHAC",
        // STRING: HYPERX 0x004bbae0
        "HBLOGO",
    };

    g_nSplashFrame++;

    if (g_nSplashFrame > (int)std::size(s_apszSplashes))
    {
        g_nSplashFrame = 0;

        WndProc::HandlePaint();

        return;
    }

    Screen::ClearBlack();

    GDI::DrawBitmapToWindow(s_apszSplashes[g_nSplashFrame - 1]);

    s_dwSplashShown = (unsigned int)SDL_GetTicks();
}

bool GameApp::UpdateSplash()
{
    if (g_nSplashFrame == 0)
    {
        return false;
    }

    if ((unsigned int)SDL_GetTicks() - s_dwSplashShown >= SPLASH_MS)
    {
        GameApp::NextSplash();
    }
    else
    {
        Window::PresentMenu();
    }

    FrameTimer::Pace();

    return true;
}

// 'x' is skipped like a space.
// FUNCTION: HYPERX 0x00419420
// The original skipped the program's name first; the arguments come without it.
void GameApp::ParseCommandLine(const char* pszArguments)
{
    const char* p = pszArguments;

    while (*p == ' ' || *p == '\r' || *p == '\n' || *p == 'x')
    {
        p = p + 1;
    }

    if (*p != '-')
    {
        return;
    }

    do
    {
        p = p + 1;

        switch (*p)
        {
            case 'f':
                g_bFullScreen = 1;
                p             = p + 1;
                break;

            case 'w':
                g_bFullScreen = 0;
                p             = p + 1;
                break;

            case 's':
                g_bNoSoundArg = 1;
                p             = p + 1;
                break;

            case 'm':
                g_bShadedDromeModels = 1;
                p                    = p + 1;
                break;
        }

        while (*p == ' ' || *p == '\r' || *p == '\n' || *p == 'x')
        {
            p = p + 1;
        }
    } while (*p == '-');
}

// FUNCTION: HYPERX 0x00419580
void GameApp::Shutdown()
{
    Gamepad::Shutdown();

    if (g_bSessionActive != 0)
    {
        GameSession::Disconnect();
    }

    Game::ShutdownEngine();

    ColorPalette* const pPalette = g_pColorPalette;

    if (pPalette != 0)
    {
        pPalette->Release();
        operator delete(pPalette);
        g_pColorPalette = 0;
    }

    SoftRenderer::FreeBackBuffer();

    if (g_pLeftStack != 0)
    {
        free(g_pLeftStack);
        g_pLeftStack = 0;
    }

    if (g_pRightStack != 0)
    {
        free(g_pRightStack);
        g_pRightStack = 0;
    }

    auto quit = SDL_Event{};

    quit.type = SDL_EVENT_QUIT;

    SDL_PushEvent(&quit);
}

// FUNCTION: HYPERX 0x00419640
void GameApp::OnStartup(void)
{
    if (g_gameApp.m_hWnd != 0)
    {
        Renderer::InitializeLightDirections();
        return;
    }

    // STRING: HYPERX 0x004bbb04
    g_gameApp.Fatal("InitApp - Null window handle.");
}

// Never returns.
// FUNCTION: HYPERX 0x00419670
void GameApp::OnExit(void)
{
    Window::Destroy();

    std::_Exit(0);
}

#define CDROM_NOT_FOUND  (-1)
#define CDROM_NOT_WANTED (-2)

// Runs before WinMain.
// FUNCTION: HYPERX 0x00422f90
GameApp::GameApp(short nStringCount)
: m_strExeDir(MAX_PATH)
, m_strAssetsPath(MAX_PATH)
, m_strAnimDataPath(MAX_PATH)
, m_strModelPath(MAX_PATH)
, m_strPlansPath(MAX_PATH)
, m_strSoundsPath(MAX_PATH)
, m_strTexturesPath(MAX_PATH)
, m_nFrameRate(0)
, m_hWnd(0)
, m_nEngineState(ESTATE_LOBBY)
, m_bIsActive(0)
, m_wRunStateOnDeactivate(ENGINE_RUN_PLAYING)
, m_bCursorWasVisible(1)
, m_hWndActiveDialog(0)
, m_stringPool(nStringCount)
{
    m_pfnShutdown = 0;
    m_nCDRomDrive = CDROM_NOT_FOUND;

    m_vLightSpherical.x = m_vLightSpherical.y = m_vLightSpherical.z = g_flAppInitZero;
}

#define CDROM_DRIVE_COUNT ('Z' - 'A' + 1)

// FUNCTION: HYPERX 0x00423140
void GameApp::SetWindowAndCallbacks(HWND hWnd, void (*pfnStartup)(void), void (*pfnShutdown)(void))
{
    m_hWnd        = hWnd;
    m_pfnShutdown = pfnShutdown;

    if (pfnStartup != 0)
    {
        pfnStartup();
    }
}

// FUNCTION: HYPERX 0x00423160
void GameApp::OnShutdown()
{
    if (g_pAudioMgr != 0)
    {
        g_pAudioMgr->StopCD();
    }

    if (m_pfnShutdown != 0)
    {
        m_pfnShutdown();
    }
}

// FUNCTION: HYPERX 0x00423180
int GameApp::FindCDRomDrive(bool bRequired)
{
    char  szCaption[MAX_PATH];
    char  szFileSystem[MAX_PATH];
    char  szLabel[MAX_PATH];
    char  szText[MAX_PATH];
    char  szVolume[MAX_PATH];
    char  szScanRoot[4];
    DWORD dwOut;
    char  szRoot[4];

    // STRING: HYPERX 0x004bc418
    memcpy(szScanRoot, "A:\\", sizeof("A:\\"));

    strcpy(szLabel, m_stringPool.LoadText(STRING_HYPERBLADE_UPPERCASE));
    strcpy(szCaption, m_stringPool.LoadText(STRING_CD_NOT_FOUND));

    if (bRequired)
    {
        strcpy(szText, m_stringPool.LoadText(STRING_INSERT_CD_OR_EXIT));
    }
    else
    {
        strcpy(szText, m_stringPool.LoadText(STRING_INSERT_CD_OR_RETURN));
    }

    int nDrive = m_nCDRomDrive;

    if (nDrive != CDROM_NOT_FOUND)
    {
        if (nDrive == CDROM_NOT_WANTED)
        {
            goto done;
        }

        // STRING: HYPERX 0x004bc418
        memcpy(szRoot, "A:\\", sizeof("A:\\"));
        szRoot[0] = (char)(szRoot[0] + nDrive);

        if (GetVolumeInformationA(szRoot, szVolume, MAX_PATH, &dwOut, &dwOut, &dwOut, szFileSystem, MAX_PATH) == 0)
        {
            szVolume[0] = 0;
        }

        if (strcmp(szVolume, szLabel) == 0)
        {
            return m_nCDRomDrive;
        }
    }

    for (;;)
    {
        nDrive        = 0;
        szScanRoot[0] = 'A';

        do
        {
            if (Win32::IsCDRomDrive(szScanRoot) != 0)
            {
                // STRING: HYPERX 0x004bc418
                memcpy(szRoot, "A:\\", sizeof("A:\\"));
                szRoot[0] = (char)(szRoot[0] + nDrive);

                if (GetVolumeInformationA(szRoot, szVolume, MAX_PATH, &dwOut, &dwOut, &dwOut, szFileSystem, MAX_PATH) == 0)
                {
                    szVolume[0] = 0;
                }

                if (strcmp(szVolume, szLabel) == 0)
                {
                    m_nCDRomDrive = nDrive;

                    goto done;
                }
            }

            szScanRoot[0]++;
            nDrive++;
        } while (nDrive < CDROM_DRIVE_COUNT);

        m_nCDRomDrive = CDROM_NOT_FOUND;

        if (GameApp::ShowMessageBox(szText, szCaption, Window::MessageButtons::OK_CANCEL) == Window::MessageAnswer::CANCEL)
        {
            if (!bRequired)
            {
                break;
            }

            OnShutdown();
        }
    }

done:
    return m_nCDRomDrive;
}

// The scan carries on over the string it has just rewritten.
// FUNCTION: HYPERX 0x004233d0
char* GameApp::ResolveCDRomPath(char* pszPath)
{
    char* p;

    if (pszPath[1] == ':' && g_gameApp.FindCDRomDrive(true) >= 0)
    {
        for (p = pszPath; *p != 0; p++)
        {
            if (SDL_strncasecmp(p, "data", 4) == 0)
            {
                char* pszDest = pszPath + 3;

                while (*p != 0)
                {
                    *pszDest = *p;
                    pszDest++;
                    p++;
                }

                *pszDest = 0;

                pszPath[0] = (char)(g_gameApp.FindCDRomDrive(false) + 'A');
            }
        }
    }

    return pszPath;
}

#define ERROR_BOX(t, c) Window::ShowMessage(t, c, Window::MessageButtons::OK, Window::MessageIcon::STOP)

// Files open through SDL, so SDL holds the reason the last one failed.
// FUNCTION: HYPERX 0x004238b0
void GameApp::ReportLastError()
{
    Error("%s", SDL_GetError());
}

// FUNCTION: HYPERX 0x00423900
void GameApp::ShowErrorBox(String strMessage)
{
    const int nCursor = GameApp::SetCursorVisible(1);

    String strCaption = m_strCaption.m_pchData;

    strCaption.AppendData(String::SafeStrlen(g_pszCaptionSeparator),
                          g_pszCaptionSeparator);

    char* const pszDetail = m_stringPool.LoadText(STRING_ALERT);

    strCaption.AppendData(String::SafeStrlen(pszDetail), pszDetail);

    ERROR_BOX(strMessage.m_pchData, strCaption.m_pchData);

    GameApp::SetCursorVisible(nCursor);
}

// FUNCTION: HYPERX 0x00423a00
void GameApp::FatalErrorFmtString(const char* pszFormat, va_list args)
{
    String strFormat(pszFormat);

    FatalErrorFmt(strFormat.m_pchData, args);
}

#define SHUT_DOWN(pApp) pApp->OnShutdown()

// FUNCTION: HYPERX 0x00423a70
void GameApp::ErrorFmt(const char* pszFormat, va_list args)
{
    String::FormatAlloc(&m_stringPool.m_strMessage, pszFormat, args);

    ShowErrorBox(m_stringPool.m_strMessage);

    m_stringPool.m_strMessage.Empty();
}

// FUNCTION: HYPERX 0x00423ab0
void GameApp::FatalErrorFmt(const char* pszFormat, va_list args)
{
    String::FormatAlloc(&m_stringPool.m_strMessage, pszFormat, args);

    FatalError(m_stringPool.m_strMessage);

    m_stringPool.m_strMessage.Empty();
}

// FUNCTION: HYPERX 0x00423af0
void GameApp::FatalError(String strMessage)
{
    ShowErrorBox(strMessage);

    SHUT_DOWN(this);
}

// FUNCTION: HYPERX 0x0042f3b0
void GameApp::FatalString(const char* pszFormat, ...)
{
    va_list argList;

    va_start(argList, pszFormat);

    FatalErrorFmtString(pszFormat, argList);

    va_end(argList);
}

// FUNCTION: HYPERX 0x0044b350
void GameApp::SetupViewports(void)
{
    int nPrevHeight;

    CameraObject* const pCamera = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);

    int nPrevWidth = g_nPrevWidth;

    if (nPrevWidth == 0 || (nPrevHeight = g_nPrevHeight) == 0)
    {
        nPrevHeight   = g_nBackBufferHeight;
        nPrevWidth    = g_nBackBufferWidth;
        g_nPrevWidth  = nPrevWidth;
        g_nPrevHeight = nPrevHeight;
    }

    // The client computes both insets before pushing either (0x0044b395 to 0x0044b3ae).
    const int nInsetY = (g_nBackBufferHeight - nPrevHeight) / 2;
    const int nInsetX = (g_nBackBufferWidth - nPrevWidth) / 2;

    pCamera->layout = Widget::InitLayoutState(g_pBackBufferBitsCopy, g_nBackBufferWidth, g_nBackBufferHeight, g_nBackBufferWidth, nPrevWidth, nPrevHeight, nInsetX, nInsetY);

    pCamera->InitViewTransforms();

    const float flScale = pCamera->m_flHalfFov;

    CameraObject* const pView = g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT);

    if (g_pRenderer == 0 || pView == 0)
    {
        return;
    }

    ViewExtent size;
    ViewExtent offset;

    size   = g_pRenderer->GetHeadCamSize();
    offset = g_pRenderer->GetHeadCamOrigin();

    // The inset is placed in the HUD layer, and the 3D under it draws at the back buffer's scale.
    offset = HudLayer::ToBackBuffer(offset);
    size.x *= HudLayer::Scale();
    size.y *= HudLayer::Scale();

    pView->layout = Widget::InitLayoutState(g_pBackBufferBitsCopy,
                                            g_nBackBufferWidth,
                                            g_nBackBufferHeight,
                                            g_nBackBufferWidth,
                                            size.x,
                                            size.y,
                                            offset.x,
                                            offset.y);

    pView->m_flHalfFov = flScale * 0.16666667f;

    // Twice in the original.
    pView->InitViewTransforms();
    pView->InitViewTransforms();
}

// FUNCTION: HYPERX 0x0044bca0
int GameApp::SetCursorVisible(int bVisible)
{
    const int bWasVisible = (g_bCursorHidden == 0);
    g_bCursorHidden       = (bVisible == 0);

    if (bVisible != 0)
    {
        SDL_EnableScreenSaver();
    }
    else
    {
        SDL_DisableScreenSaver();
    }

    return bWasVisible;
}

// FUNCTION: HYPERX 0x0044c160
int GameApp::MainUpdate()
{
    // A finished track is followed by the menu track, or a random one during play.
    if (g_soundMixer.Update() && g_pAudioMgr != 0)
    {
        if (g_pGameEngine == 0 || g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE || (short)g_pGameEngine->m_wRunState != ENGINE_RUN_PLAYING)
        {
            if (g_pGameOptions != 0 && g_pGameOptions->m_nInstallLevel != 0)
            {
                g_pAudioMgr->PlayCDTrack(CD_TRACK_MENU);
            }
        }
        else
        {
            g_pAudioMgr->PlayCDTrack(CD_TRACK_RANDOM);
        }
    }

    if (g_gameApp.m_bIsActive)
    {
        if (GameApp::UpdateSplash())
        {
            return 0;
        }

        const auto bUncapped = g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE && GraphicsSettings::On(GraphicsSettings::Switch::UNCAPPED);

        if (g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
        {
            Game::UpdateState();
        }
        else if (bUncapped)
        {
            Game::Draw(FramePacing::RunTicks());
            g_nPrevEngineState = ESTATE_MATCH_LIVE;
        }
        else
        {
            Game::RunFrame();
            g_nPrevEngineState = ESTATE_MATCH_LIVE;
        }

        FrameTimer::EndFrameUpdate(&g_frameTimer);

        // Nothing in the original waits for the frame, and a modern machine runs it too fast.
        if (!bUncapped)
        {
            FrameTimer::Pace();
        }
    }

    return g_gameApp.m_bIsActive == 0;
}

// FUNCTION: HYPERX 0x0046eea0
int GameApp::GetInstallLevel(void)
{
    return g_nInstallLevel;
}

// FUNCTION: HYPERX 0x0047adb0
Window::MessageAnswer GameApp::ShowMessageBox(const char* pszText, const char* pszCaption, Window::MessageButtons buttons)
{
    const int bWasVisible = GameApp::SetCursorVisible(1);

    const auto nResult = Window::ShowMessage(pszText, pszCaption, buttons, Window::MessageIcon::INFO);

    GameApp::SetCursorVisible(bWasVisible);

    return nResult;
}

// Returning zero tells the runtime not to retry.
// FUNCTION: HYPERX 0x0047adf0
int GameApp::OutOfMemoryHandler(size_t)
{
    // STRING: HYPERX 0x004c682c
    g_gameApp.Fatal("In C++ \"new\" method: Memory pool empty!");

    return 0;
}

// FUNCTION: HYPERX 0x0048ceb0
void GameApp::ResetToSinglePlayer(void)
{
    g_nMainMenuTopChoice   = 1;
    g_nMainMenuMatchChoice = 0;
    g_nPlayerMode          = PLAYER_MODE_EXHIBITION;
    g_nMainMenuPage        = 0;
}

// A failed allocation is stored through unchecked, as the original does.
// FUNCTION: HYPERX 0x004922f0
void GameApp::InstantiateState(void)
{
    int                nReturnState;
    MatchInfoUIScreen* pScreen;

    if (g_pScreenMatchInfo != 0)
    {
        nReturnState            = g_pScreenMatchInfo->m_nNextState;
        g_nMatchInfoReturnState = nReturnState;
    }

    if (g_pScreenSaveLoad != 0)
    {
        nReturnState           = g_pScreenSaveLoad->m_nReturnState;
        g_nSaveLoadReturnState = nReturnState;
    }

    if (g_pScreenGameConfig != 0)
    {
        nReturnState             = g_pScreenGameConfig->m_nReturnState;
        g_nGameConfigReturnState = nReturnState;
    }

    delete g_pActiveScreen;

    g_pActiveScreen = 0;

    if (g_pScreenMatchInfo != 0)
    {
        g_pScreenMatchInfo = 0;
    }

    if (g_pScreenSaveLoad != 0)
    {
        g_pScreenSaveLoad = 0;
    }

    if (g_pScreenGameConfig != 0)
    {
        g_pScreenGameConfig = 0;
    }

    switch (g_gameApp.m_nEngineState)
    {
        case ESTATE_LOBBY:
            g_pActiveScreen = new LobbyScreen;
            break;

        case ESTATE_MAIN_MENU:
            g_pActiveScreen = new MainMenuScreen;
            break;

        case ESTATE_SAVE_LOAD:
            g_pScreenSaveLoad = new SaveLoadUIScreen;

            if (g_nPrevEngineState != ESTATE_PAUSE_MINIMAL)
            {
                g_pScreenSaveLoad->m_nReturnState = g_nPrevEngineState;
                g_pActiveScreen                   = g_pScreenSaveLoad;

                break;
            }

            g_pScreenSaveLoad->m_nReturnState = g_nSaveLoadReturnState;
            g_pActiveScreen                   = g_pScreenSaveLoad;
            break;

        case ESTATE_TOURNAMENT_SETUP:
            g_pActiveScreen = new TournamentSetupUIScreen;
            break;

        case ESTATE_HEAD_TO_HEAD_SETUP:
            g_pActiveScreen = new HeadToHeadSetupUIScreen;
            break;

        case ESTATE_MATCH_INFO:
            g_pScreenMatchInfo = new MatchInfoUIScreen;
            nReturnState       = g_nPrevEngineState;

            if (g_nPrevEngineState == ESTATE_PAUSE_MINIMAL || g_nPrevEngineState == ESTATE_SAVE_LOAD)
            {
                nReturnState = g_nMatchInfoReturnState;
            }

            g_pScreenMatchInfo->m_nNextState = nReturnState;
            g_pActiveScreen                  = g_pScreenMatchInfo;
            break;

        case ESTATE_CHAMPIONSHIP_SETUP:
            pScreen               = new ChampionshipSetupUIScreen;
            pScreen->m_nNextState = g_nMatchInfoReturnState;
            g_pActiveScreen       = pScreen;
            break;

        case ESTATE_LEAGUE_SETUP:
            pScreen               = new LeagueSetupUIScreen;
            pScreen->m_nNextState = g_nMatchInfoReturnState;
            g_pActiveScreen       = pScreen;
            break;

        case ESTATE_GAME_CLIPS_VIEWER:
            // STRING: HYPERX 0x004c8c2c
            g_gameApp.Fatal("Invalid state detected: GameClipsViewer\n");
            break;

        case ESTATE_AWARDS:
            pScreen               = new AwardsUIScreen;
            pScreen->m_nNextState = g_nMatchInfoReturnState;
            g_pActiveScreen       = pScreen;
            break;

        case ESTATE_GAME_CONFIG:
            g_pScreenGameConfig = new GameConfigUIScreen;
            nReturnState        = g_nPrevEngineState;

            if (g_nPrevEngineState == ESTATE_PAUSE_MINIMAL || g_nPrevEngineState == ESTATE_KEY_CONFIG)
            {
                nReturnState = g_nGameConfigReturnState;
            }

            g_pScreenGameConfig->m_nReturnState = nReturnState;
            g_pActiveScreen                     = g_pScreenGameConfig;
            break;

        case ESTATE_KEY_CONFIG:
            g_pActiveScreen = new KeyConfigScreen;
            break;

        case ESTATE_DROME_EXTERIOR:
            g_pActiveScreen = new DromeExteriorScreen;
            break;

        case ESTATE_JUMBOTRON:
            g_pActiveScreen = new JumbotronScreen;
            break;

        case ESTATE_PAUSE_FULL:
            g_pActiveScreen = new PauseMenuFullScreen;
            break;

        case ESTATE_PAUSE_MINIMAL:
            g_pActiveScreen = new PauseMenuMinimalScreen;
            break;

        case ESTATE_PAUSE_NO_SAVE:
            g_pActiveScreen = new PauseMenuNoSaveScreen;
            break;

        case 1:
        case 2:
        case ESTATE_MATCH_LIVE:
            g_pActiveScreen = 0;

            return;

        case ESTATE_MULTI_SETUP_OPTIONS:
            g_pActiveScreen = new MultiSetupOptionsScreen;
            break;

        case ESTATE_FINAL_CEREMONY:
            g_pActiveScreen = new FinalCeremonyScreen;
            break;

        case ESTATE_GAME_INTRO:
            g_pActiveScreen = new GameIntroScreen;
            break;

        case ESTATE_TRAINING_END:
            g_pActiveScreen = new TrainingEndScreen;
            break;

        default:
            return;
    }

    if (g_pActiveScreen != 0)
    {
        g_pActiveScreen->InitFrame();
    }
}

// FUNCTION: HYPERX 0x00493870
HWND GameApp::CreateMainWindow(const char* pszTitle)
{
    const HWND hWnd = Window::Create(pszTitle);

    // What WM_CREATE did before SDL made the window.
    if (hWnd == 0 || MainWindow::OnCreate() == 0)
    {
        return 0;
    }

    return hWnd;
}
