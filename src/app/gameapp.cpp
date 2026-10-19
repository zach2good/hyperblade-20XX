#include "common/compat.h"
#include "engine/spectator.h"
#include "app/simulator.h"
#include "common/clock.h"
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
#include "common/pak.h"
#include "common/stringtable.h"
#include <SDL3/SDL.h>
#include "graphics/gdi.h"
#include "graphics/graphics.h"
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
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cstdio>
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
#include <new>
#include "graphics/display.h"
#include "ui/infobarbase.h"
#include "graphics/cgpalette.h"
#include "input/input.h"
#include "app/cddrive.h"
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
bool GameApp::Initialize(const char* pszArguments)
{
    s_arguments = pszArguments;

    const char* pszAppClassName;

    std::set_new_handler(GameApp::OutOfMemoryHandler);
    // The text comes from the player's own HYPERX.EXE the first time, and from the strings file after.
    const auto* const pszBase = SDL_GetBasePath();

    if (pszBase == nullptr || !StringTable::Load(pszBase))
    {
        Window::ShowMessage("HyperBlade 20XX runs from your HyperBlade folder. Put hyperblade_20XX.exe beside HYPERX.EXE and start it from there.", "HyperBlade 20XX", Window::MessageButtons::OK, Window::MessageIcon::STOP);

        std::_Exit(1);
    }

    if (!Pak::Open((std::string(pszBase) + Pak::kFileName).c_str()))
    {
        Window::ShowMessage("HyperBlade 20XX is missing 20XX_data.pak. Keep it beside hyperblade_20XX.exe.", "HyperBlade 20XX", Window::MessageButtons::OK, Window::MessageIcon::STOP);

        std::_Exit(1);
    }

    GameApp::InitializeResourcePaths();
    GameApp::ParseCommandLine(pszArguments);

    // A modern desktop keeps its own mode, so the game runs windowed.
    g_bFullScreen = 0;

    g_pColorPalette = std::make_unique<ColorPalette>();

    pszAppClassName    = g_gameApp.m_strCaption.m_pchData;
    const auto bWindow = GameApp::CreateMainWindow(pszAppClassName);

    g_gameApp.SetWindowAndCallbacks(bWindow, GameApp::OnStartup, GameApp::OnExit);

    g_gameApp.m_bCursorWasVisible = Display::Initialize();

    // A simulation never shows its window.
    if (!Simulator::Requested())
    {
        Window::Show();
        GameApp::SetCursorVisible(1);

        g_nSplashFrame = 0;
        GameApp::NextSplash();
    }

    // Windows shows its start-up busy cursor until a new program first takes its messages.
    Window::Pump();

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

    return bWindow;
}

// FUNCTION: HYPERX 0x00419070
void GameApp::InitializeResourcePaths()
{
    auto* const pszClassName = g_gameApp.m_stringPool.LoadText(kStringHyperBlade);

    g_gameApp.m_strCaption.Assign(String::SafeStrlen(pszClassName), pszClassName);

    PathString strDirectory(kMaxPathLength);

    // SDL's base path ends in a separator.
    const auto* const pszBase = SDL_GetBasePath();

    if (pszBase != nullptr)
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

constexpr auto kSplashTime = std::chrono::seconds{ 3 };

static Clock::Milliseconds s_splashShown{};

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

    if (g_nSplashFrame > static_cast<int>(std::size(s_apszSplashes)))
    {
        g_nSplashFrame = 0;

        WndProc::HandlePaint();

        return;
    }

    Screen::ClearBlack();

    GDI::DrawBitmapToWindow(s_apszSplashes[g_nSplashFrame - 1]);

    s_splashShown = Clock::Now();
}

bool GameApp::UpdateSplash()
{
    if (g_nSplashFrame == 0)
    {
        return false;
    }

    if (Clock::Now() - s_splashShown >= kSplashTime)
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
    const auto* p = pszArguments;

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

            // Bots play every dromer, the player's included.
            case 'b':
                Spectator::SetAutopilot(true);
                p = p + 1;
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

    // Freed here, before the engine, rather than by the static destructors after main.
    g_pActiveScreen = nullptr;

    Game::ShutdownEngine();

    if (g_pColorPalette != nullptr)
    {
        g_pColorPalette->Release();
        g_pColorPalette = nullptr;
    }

    SoftRenderer::FreeBackBuffer();

    Graphics::FreeEdgeStacks();

    auto quit = SDL_Event{};

    quit.type = SDL_EVENT_QUIT;

    SDL_PushEvent(&quit);
}

// FUNCTION: HYPERX 0x00419640
void GameApp::OnStartup()
{
    if (g_gameApp.m_bHasWindow)
    {
        Renderer::InitializeLightDirections();
        return;
    }

    // STRING: HYPERX 0x004bbb04
    g_gameApp.Fatal("InitApp - Null window handle.");
}

// Never returns.
// FUNCTION: HYPERX 0x00419670
void GameApp::OnExit()
{
    Window::Destroy();

    std::_Exit(0);
}

constexpr auto kCdromNotFound  = (-1);
constexpr auto kCdromNotWanted = (-2);

// Runs before WinMain.
// FUNCTION: HYPERX 0x00422f90
GameApp::GameApp(short nStringCount)
: m_strExeDir(kMaxPathLength)
, m_strAssetsPath(kMaxPathLength)
, m_strAnimDataPath(kMaxPathLength)
, m_strModelPath(kMaxPathLength)
, m_strPlansPath(kMaxPathLength)
, m_strSoundsPath(kMaxPathLength)
, m_strTexturesPath(kMaxPathLength)
, m_nFrameRate(0)
, m_bHasWindow(false)
, m_nEngineState(kEstateLobby)
, m_bIsActive(0)
, m_wRunStateOnDeactivate(kEngineRunPlaying)
, m_bCursorWasVisible(1)
, m_stringPool(nStringCount)
{
    m_pfnShutdown = nullptr;
    m_nCDRomDrive = kCdromNotFound;

    m_vLightSpherical.x = m_vLightSpherical.y = m_vLightSpherical.z = g_flAppInitZero;
}

#define CDROM_DRIVE_COUNT ('Z' - 'A' + 1)

// FUNCTION: HYPERX 0x00423140
void GameApp::SetWindowAndCallbacks(bool bHasWindow, void (*pfnStartup)(), void (*pfnShutdown)())
{
    m_bHasWindow  = bHasWindow;
    m_pfnShutdown = pfnShutdown;

    if (pfnStartup != nullptr)
    {
        pfnStartup();
    }
}

// FUNCTION: HYPERX 0x00423160
void GameApp::OnShutdown()
{
    if (g_pAudioMgr != nullptr)
    {
        g_pAudioMgr->StopCD();
    }

    if (m_pfnShutdown != nullptr)
    {
        m_pfnShutdown();
    }
}

// FUNCTION: HYPERX 0x00423180
int GameApp::FindCDRomDrive(bool bRequired)
{
    char szCaption[kMaxPathLength];
    char szLabel[kMaxPathLength];
    char szText[kMaxPathLength];

    strcpy(szLabel, m_stringPool.LoadText(kStringHyperBladeUppercase));
    strcpy(szCaption, m_stringPool.LoadText(kStringCDNotFound));

    if (bRequired)
    {
        strcpy(szText, m_stringPool.LoadText(kStringInsertCDOrExit));
    }
    else
    {
        strcpy(szText, m_stringPool.LoadText(kStringInsertCDOrReturn));
    }

    // Ripped music plays in place of the disc's tracks, so the disc is only looked for without it,
    // and only where the platform can look.
    if (m_nCDRomDrive == kCdromNotFound && (AudioMgr::HasRippedMusic() || !CdDrive::Supported()))
    {
        m_nCDRomDrive = kCdromNotWanted;
    }

    auto nDrive = m_nCDRomDrive;

    if (nDrive != kCdromNotFound)
    {
        if (nDrive == kCdromNotWanted)
        {
            goto done;
        }

        if (CdDrive::VolumeLabel(nDrive) == szLabel)
        {
            return m_nCDRomDrive;
        }
    }

    for (;;)
    {
        nDrive = 0;

        do
        {
            if (CdDrive::IsCDRom(nDrive) && CdDrive::VolumeLabel(nDrive) == szLabel)
            {
                m_nCDRomDrive = nDrive;

                goto done;
            }

            nDrive++;
        } while (nDrive < CDROM_DRIVE_COUNT);

        m_nCDRomDrive = kCdromNotFound;

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
                auto* pszDest = pszPath + 3;

                while (*p != 0)
                {
                    *pszDest = *p;
                    pszDest++;
                    p++;
                }

                *pszDest = 0;

                pszPath[0] = static_cast<char>(g_gameApp.FindCDRomDrive(false) + 'A');
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
    const auto nCursor = GameApp::SetCursorVisible(1);

    String strCaption = m_strCaption.m_pchData;

    strCaption.AppendData(String::SafeStrlen(g_pszCaptionSeparator),
                          g_pszCaptionSeparator);

    auto* const pszDetail = m_stringPool.LoadText(kStringAlert);

    strCaption.AppendData(String::SafeStrlen(pszDetail), pszDetail);

    // A simulation runs unattended, so it reports to its output.
    if (Simulator::Requested())
    {
        std::printf("error: %s\n", strMessage.m_pchData);
        std::fflush(stdout);
    }
    else
    {
        ERROR_BOX(strMessage.m_pchData, strCaption.m_pchData);
    }

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

    // The matches already written stand; the batch reruns the rest.
    if (Simulator::Requested())
    {
        std::_Exit(3);
    }

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
void GameApp::SetupViewports()
{
    int nPrevHeight;

    auto* const pCamera = g_pGameEngine->GetCamera(kCameraMainSlot);

    auto nPrevWidth = g_nPrevWidth;

    if (nPrevWidth == 0 || (nPrevHeight = g_nPrevHeight) == 0)
    {
        nPrevHeight   = g_nBackBufferHeight;
        nPrevWidth    = g_nBackBufferWidth;
        g_nPrevWidth  = nPrevWidth;
        g_nPrevHeight = nPrevHeight;
    }

    // The client computes both insets before pushing either (0x0044b395 to 0x0044b3ae).
    const auto nInsetY = (g_nBackBufferHeight - nPrevHeight) / 2;
    const auto nInsetX = (g_nBackBufferWidth - nPrevWidth) / 2;

    pCamera->layout = Widget::InitLayoutState(g_pBackBufferBitsCopy, g_nBackBufferWidth, g_nBackBufferHeight, g_nBackBufferWidth, nPrevWidth, nPrevHeight, nInsetX, nInsetY);

    pCamera->InitViewTransforms();

    const auto flScale = pCamera->m_flHalfFov;

    auto* const pView = g_pGameEngine->GetCamera(kCameraHeadSlot);

    if (g_pRenderer == nullptr || pView == nullptr)
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
    if (g_soundMixer.Update() && g_pAudioMgr != nullptr)
    {
        if (g_pGameEngine == nullptr || g_gameApp.m_nEngineState != kEstateMatchLive || static_cast<short>(g_pGameEngine->m_wRunState) != kEngineRunPlaying)
        {
            if (g_pGameOptions != nullptr && g_pGameOptions->m_nInstallLevel != 0)
            {
                g_pAudioMgr->PlayCDTrack(kCDTrackMenu);
            }
        }
        else
        {
            g_pAudioMgr->PlayCDTrack(kCDTrackRandom);
        }
    }

    if (g_gameApp.m_bIsActive)
    {
        if (GameApp::UpdateSplash())
        {
            return 0;
        }

        const auto bUncapped = g_gameApp.m_nEngineState == kEstateMatchLive && GraphicsSettings::On(GraphicsSettings::Switch::UNCAPPED);

        if (g_gameApp.m_nEngineState != kEstateMatchLive)
        {
            FramePacing::Reset();
            Game::UpdateState();
        }
        else if (bUncapped)
        {
            Game::Draw(FramePacing::RunTicks());
            g_nPrevEngineState = kEstateMatchLive;
        }
        else
        {
            Game::RunFrame();
            g_nPrevEngineState = kEstateMatchLive;
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
int GameApp::GetInstallLevel()
{
    return g_nInstallLevel;
}

// FUNCTION: HYPERX 0x0047adb0
Window::MessageAnswer GameApp::ShowMessageBox(const char* pszText, const char* pszCaption, Window::MessageButtons buttons)
{
    const auto bWasVisible = GameApp::SetCursorVisible(1);

    const auto nResult = Window::ShowMessage(pszText, pszCaption, buttons, Window::MessageIcon::INFO);

    GameApp::SetCursorVisible(bWasVisible);

    return nResult;
}

// Returning zero tells the runtime not to retry.
// FUNCTION: HYPERX 0x0047adf0
void GameApp::OutOfMemoryHandler()
{
    // STRING: HYPERX 0x004c682c
    g_gameApp.Fatal("In C++ \"new\" method: Memory pool empty!");

    // The original's handler answered nought, failing the allocation.
    throw std::bad_alloc();
}

// FUNCTION: HYPERX 0x0048ceb0
void GameApp::ResetToSinglePlayer()
{
    g_nMainMenuTopChoice   = 1;
    g_nMainMenuMatchChoice = 0;
    g_nPlayerMode          = kPlayerModeExhibition;
    g_nMainMenuPage        = 0;
}

// A failed allocation is stored through unchecked, as the original does.
// FUNCTION: HYPERX 0x004922f0
void GameApp::InstantiateState()
{
    int nReturnState;

    if (g_pScreenMatchInfo != nullptr)
    {
        nReturnState            = g_pScreenMatchInfo->m_nNextState;
        g_nMatchInfoReturnState = nReturnState;
    }

    if (g_pScreenSaveLoad != nullptr)
    {
        nReturnState           = g_pScreenSaveLoad->m_nReturnState;
        g_nSaveLoadReturnState = nReturnState;
    }

    if (g_pScreenGameConfig != nullptr)
    {
        nReturnState             = g_pScreenGameConfig->m_nReturnState;
        g_nGameConfigReturnState = nReturnState;
    }

    g_pActiveScreen = nullptr;

    if (g_pScreenMatchInfo != nullptr)
    {
        g_pScreenMatchInfo = nullptr;
    }

    if (g_pScreenSaveLoad != nullptr)
    {
        g_pScreenSaveLoad = nullptr;
    }

    if (g_pScreenGameConfig != nullptr)
    {
        g_pScreenGameConfig = nullptr;
    }

    switch (g_gameApp.m_nEngineState)
    {
        case kEstateLobby:
            g_pActiveScreen = std::make_unique<LobbyScreen>();
            break;

        case kEstateMainMenu:
            g_pActiveScreen = std::make_unique<MainMenuScreen>();
            break;

        case kEstateSaveLoad:
        {
            auto pScreen = std::make_unique<SaveLoadUIScreen>();

            g_pScreenSaveLoad = pScreen.get();

            if (g_nPrevEngineState != kEstatePauseMinimal)
            {
                g_pScreenSaveLoad->m_nReturnState = g_nPrevEngineState;
                g_pActiveScreen                   = std::move(pScreen);

                break;
            }

            g_pScreenSaveLoad->m_nReturnState = g_nSaveLoadReturnState;
            g_pActiveScreen                   = std::move(pScreen);
            break;
        }

        case kEstateTournamentSetup:
            g_pActiveScreen = std::make_unique<TournamentSetupUIScreen>();
            break;

        case kEstateHeadToHeadSetup:
            g_pActiveScreen = std::make_unique<HeadToHeadSetupUIScreen>();
            break;

        case kEstateMatchInfo:
        {
            auto pScreen = std::make_unique<MatchInfoUIScreen>();

            g_pScreenMatchInfo = pScreen.get();
            nReturnState       = g_nPrevEngineState;

            if (g_nPrevEngineState == kEstatePauseMinimal || g_nPrevEngineState == kEstateSaveLoad)
            {
                nReturnState = g_nMatchInfoReturnState;
            }

            g_pScreenMatchInfo->m_nNextState = nReturnState;
            g_pActiveScreen                  = std::move(pScreen);
            break;
        }

        case kEstateChampionshipSetup:
        {
            auto pScreen = std::make_unique<ChampionshipSetupUIScreen>();

            pScreen->m_nNextState = g_nMatchInfoReturnState;
            g_pActiveScreen       = std::move(pScreen);
            break;
        }

        case kEstateLeagueSetup:
        {
            auto pScreen = std::make_unique<LeagueSetupUIScreen>();

            pScreen->m_nNextState = g_nMatchInfoReturnState;
            g_pActiveScreen       = std::move(pScreen);
            break;
        }

        case kEstateGameClipsViewer:
            // STRING: HYPERX 0x004c8c2c
            g_gameApp.Fatal("Invalid state detected: GameClipsViewer\n");
            break;

        case kEstateAwards:
        {
            auto pScreen = std::make_unique<AwardsUIScreen>();

            pScreen->m_nNextState = g_nMatchInfoReturnState;
            g_pActiveScreen       = std::move(pScreen);
            break;
        }

        case kEstateGameConfig:
        {
            auto pScreen = std::make_unique<GameConfigUIScreen>();

            g_pScreenGameConfig = pScreen.get();
            nReturnState        = g_nPrevEngineState;

            if (g_nPrevEngineState == kEstatePauseMinimal || g_nPrevEngineState == kEstateKeyConfig)
            {
                nReturnState = g_nGameConfigReturnState;
            }

            g_pScreenGameConfig->m_nReturnState = nReturnState;
            g_pActiveScreen                     = std::move(pScreen);
            break;
        }

        case kEstateKeyConfig:
            g_pActiveScreen = std::make_unique<KeyConfigScreen>();
            break;

        case kEstateDromeExterior:
            g_pActiveScreen = std::make_unique<DromeExteriorScreen>();
            break;

        case kEstateJumbotron:
            g_pActiveScreen = std::make_unique<JumbotronScreen>();
            break;

        case kEstatePauseFull:
            g_pActiveScreen = std::make_unique<PauseMenuFullScreen>();
            break;

        case kEstatePauseMinimal:
            g_pActiveScreen = std::make_unique<PauseMenuMinimalScreen>();
            break;

        case kEstatePauseNoSave:
            g_pActiveScreen = std::make_unique<PauseMenuNoSaveScreen>();
            break;

        case 1:
        case 2:
        case kEstateMatchLive:
            g_pActiveScreen = nullptr;

            return;

        case kEstateMultiSetupOptions:
            g_pActiveScreen = std::make_unique<MultiSetupOptionsScreen>();
            break;

        case kEstateFinalCeremony:
            g_pActiveScreen = std::make_unique<FinalCeremonyScreen>();
            break;

        case kEstateGameIntro:
            g_pActiveScreen = std::make_unique<GameIntroScreen>();
            break;

        case kEstateTrainingEnd:
            g_pActiveScreen = std::make_unique<TrainingEndScreen>();
            break;

        default:
            return;
    }

    if (g_pActiveScreen != nullptr)
    {
        g_pActiveScreen->InitFrame();
    }
}

// FUNCTION: HYPERX 0x00493870
bool GameApp::CreateMainWindow(const char* pszTitle)
{
    // What WM_CREATE did before SDL made the window.
    return Window::Create(pszTitle) && MainWindow::OnCreate() != 0;
}
