// The game at the top level: engine setup and teardown, one frame, and the state machine.

#include "math/squirrel5.h"
#include "common/compat.h"
#include "common/clock.h"
#include <SDL3/SDL.h>
#include "graphics/hudlayer.h"
#include "globals.h"
#include <cstdlib>
#include "sound/audiomgr.h"
#include "gameoptions.h"
#include "ui/screen.h"
#include "app/gameapp.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "ui/saveobject.h"
#include "ui/buttonwidget.h"
#include "graphics/colorpalette.h"
#include "graphics/renderer.h"
#include "common/stringpool.h"
#include "graphics/graphicsglobals.h"
#include "app/frametimer.h"
#include "anim/animscripttable.h"
#include "graphics/cd3dtexturemanager.h"
#include "messagequeue.h"
#include "ui/infobarbase.h"
#include "arenastate.h"
#include "teamsblock.h"
#include "physics/physicsmarkers.h"
#include "common/staticinit.h"
#include "physics/collisionellipsoid.h"
#include "graphics/display.h"
#include "engine/game.h"
#include "engine/framepacing.h"
#include "app/window.h"
#include "common/stub.h"
#include "app/stringids.h"
#include "sound/soundids.h"
#include "engine/messageids.h"
#include "engine/objectclasses.h"
#include "planvm/damagegate.h"
#include "engine/careerstats.h"

float g_flUnusedGame                = g_flZeroGame;
Vec3  g_vBirdsEyeTrackOffset        = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vFirstPersonTrackOffset     = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vOverShoulderTrackOffset    = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vOverShoulderRokTrackOffset = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vFallTrackOffset            = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vNoOffset                   = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vZeroMarker                 = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };

// FUNCTION: HYPERX 0x0044c1b0
void Game::RunFrame()
{
    Game::Tick();
    Game::Draw(1.0f);
}

void Game::Tick()
{
    if (g_frameTimer.bFrameTimesOn)
    {
        g_frameTimer.nClockReads++;
        g_frameGameLogicStart        = Clock::Now();
        g_frameTimer.frameStartTicks = g_frameGameLogicStart;
    }
    else
    {
        g_frameGameLogicStart = Clock::Now();
    }

    if (g_pRenderer != nullptr)
    {
        g_pRenderer->BeginTick();
    }

    FramePacing::BeginTick();

    g_pGameEngine->PerFrameUpdate();

    if (g_frameTimer.bFrameTimesOn != 0)
    {
        g_frameTimer.nPerfSampleCount++;
        g_frameTimer.accGameLogic += Clock::Now() - g_frameTimer.frameStartTicks;
    }
}

void Game::Draw(float flAlpha)
{
    FramePacing::DrawViews(flAlpha);

    if (g_pRenderer != nullptr)
    {
        // The HUD draws at a fraction of the window and is scaled up over the 3D.
        HudLayer::Enter();
        HudLayer::Clear();

        g_pRenderer->DrawScene(g_renderFlags.bDisplayHUDArt);
        // The original passes no DC; src/ui/infobar.cpp makes the back buffer the text DC.
        g_pRenderer->DrawHUD(g_renderFlags.bDisplayHUDArt);

        HudLayer::Leave();
    }

    Renderer::PresentFrame();

    if (g_frameTimer.bFrameTimesOn != 0)
    {
        g_frameTimer.nPerfSampleCount++;
        g_frameTimer.accRender += Clock::Now() - g_frameTimer.frameStartTicks;
    }

    g_frameTimer.nFrames++;
}

// Builds the game's owners in the original's order, timing each stage.
// FUNCTION: HYPERX 0x0045c700
void Game::InitializeEngine()
{
    auto nStart = GameApp::GetInstallLevel();

    CareerStats::Open(String::Concat(g_gameApp.m_strExeDir.m_str, "hyperblade_20XX_career.txt").m_pchData);

    // A new of a type with a constructor, for the null test and exception frame.
    g_pTexManager = std::make_unique<CD3DTextureManager>();

    auto nDelta = GameApp::GetInstallLevel() - nStart;

    g_pTexManager->m_nInstallLevel += nDelta;

    g_pGameOptions = std::make_unique<GameOptions>();

    nStart = GameApp::GetInstallLevel();

    g_pGameEngine = std::make_unique<GameEngine>();

    nDelta = GameApp::GetInstallLevel() - nStart;

    g_pGameEngine->m_nLoadCost += nDelta;

    nStart = GameApp::GetInstallLevel();

    g_pAudioMgr = std::make_unique<AudioMgr>();

    g_pAudioMgr->Init();

    nDelta = GameApp::GetInstallLevel() - nStart;

    g_pAudioMgr->nInitTimeMs += nDelta;

    g_pGameEngine->Reset();

    Game::InitMatchMarkers();

    nStart = GameApp::GetInstallLevel();

    g_pAnimRegistry = std::make_unique<AnimRegistry>();

    // STRING: HYPERX 0x004c41c0
    g_pAnimRegistry->LoadFromPath("animdata\\AnimAssm.txt");

    nDelta = GameApp::GetInstallLevel() - nStart;

    g_pAnimRegistry->m_nLoadCost += nDelta;

    nStart = GameApp::GetInstallLevel();

    g_pRenderer = InfoBarBase::Create();

    // STRING: HYPERX 0x004c41b4
    g_pGameEngine->Init("game.cfg");

    g_pGameEngine->StartMatch();

    nDelta = GameApp::GetInstallLevel() - nStart;

    g_pGameEngine->m_nLoadCost += nDelta;

    g_pMessageQueue = std::make_unique<MessageQueue>();

    // STRING: HYPERX 0x004c41a4
    g_pMessageQueue->LoadList("infobtxt.lst");
}

// The message queue is emptied and kept, as the original keeps it.
// FUNCTION: HYPERX 0x0045c9c0
void Game::ShutdownEngine()
{
    if (g_pMessageQueue != nullptr)
    {
        g_pMessageQueue->FreeAll();
    }

    g_pRenderer.reset();
    g_pAnimRegistry.reset();
    g_pAudioMgr.reset();
    g_pGameEngine.reset();
    g_pGameOptions.reset();
    g_pTexManager.reset();
}

// Eight markers against the arena ellipsoid; the y-minus one asks for its height at y = +6, not -6.
// FUNCTION: HYPERX 0x0047d1c0
void Game::InitMatchMarkers()
{
    g_vMarkerRimEast.x = g_arenaEllipsoid.GetRadiusX() + 2.0f;

    g_vMarkerGoalNorth.y = g_arenaEllipsoid.GetRadiusY() * 0.75f;

    g_vMarkerRimEast.y   = g_flZeroPlanArg;
    g_vMarkerRimEast.z   = 10.0f;
    g_vMarkerGoalNorth.z = g_flZeroPlanArg;
    g_vMarkerGoalNorth.x = g_flZeroPlanArg;
    g_vMarkerGoalSouth.z = g_flZeroPlanArg;
    g_vMarkerGoalSouth.x = g_flZeroPlanArg;

    g_vMarkerGoalSouth.y = g_arenaEllipsoid.GetRadiusY() * -0.75f;

    g_vMarkerSurfaceYPlus.y = 6.0f;
    g_vMarkerSurfaceYPlus.x = g_arenaEllipsoid.GetRadiusX() - 0.2f;
    g_vMarkerSurfaceYPlus.z = g_arenaEllipsoid.GetZ(g_arenaEllipsoid.GetRadiusX() - 0.2f, 6.0f);

    g_vMarkerSurfaceYMinus.x = g_arenaEllipsoid.GetRadiusX() - 0.2f;
    g_vMarkerSurfaceYMinus.y = -6.0f;
    g_vMarkerSurfaceYMinus.z = g_arenaEllipsoid.GetZ(g_arenaEllipsoid.GetRadiusX() - 0.2f, 6.0f);

    g_vMarkerSurfaceCentre.x = g_arenaEllipsoid.GetRadiusX() - 4.0f;
    g_vMarkerSurfaceCentre.y = g_flZeroPlanArg;
    g_vMarkerSurfaceCentre.z = g_arenaEllipsoid.GetZ(
        g_arenaEllipsoid.GetRadiusX() - 4.0f, g_flZeroPlanArg);

    g_vMarkerGoalMouthNorth.y = g_arenaEllipsoid.GetRadiusY() - 3.0f;
    g_vMarkerGoalMouthSouth.z = -14.0f;
    g_vMarkerGoalMouthSouth.x = g_flZeroPlanArg;
    g_vMarkerGoalMouthNorth.x = g_flZeroPlanArg;
    g_vMarkerGoalMouthNorth.z = -14.0f;
    g_vMarkerGoalMouthSouth.y = 3.0f - g_arenaEllipsoid.GetRadiusY();
}

// FUNCTION: HYPERX 0x00491dd0
extern "C" unsigned int Game::SetEngineState(unsigned int state)
{
    const auto previous = g_gameApp.m_nEngineState;

    g_gameApp.m_nEngineState = state;

    return previous;
}

// Runs on the frame the state changed; every other frame only ticks the current screen.
// FUNCTION: HYPERX 0x00491de0
unsigned int Game::UpdateState()
{
    char* pszText;

    if (g_gameApp.m_nEngineState != static_cast<unsigned int>(g_nPrevEngineState))
    {
        if (g_gameApp.m_nEngineState != kEstateMatchStart)
        {
            g_pAudioMgr->PreloadGroup(kSoundGroupPreload);
        }

        if (g_nPrevEngineState == kEstateInit || g_nPrevEngineState == kEstateMatchLive)
        {
            // Reached with the manager in ECX, which the function ignores.
            g_pAudioMgr->StopAllSfx();
            g_pAudioMgr->PlaySpecific(kSoundSxxamba2);

            if (g_pGameOptions != nullptr && g_pGameOptions->m_nInstallLevel != 0)
            {
                g_pAudioMgr->PlayCDTrack(kCDTrackMenu);
            }
            else
            {
                g_pAudioMgr->StopCD();
            }

            if (g_nPrevEngineState == kEstateMatchLive)
            {
                Screen::ClearBlack();

                if (g_bFullScreen != 0)
                {
                    Display::ChangeColorDepth(2);
                }
            }
        }

        switch (g_gameApp.m_nEngineState)
        {
            case kEstateGameConfig:
                if (g_nPrevEngineState != kEstateKeyConfig)
                {
                    Screen::ClearBlack();
                }

                GameApp::InstantiateState();

                break;

            case 1:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case 2:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateTournamentSetup:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateHeadToHeadSetup:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateMatchInfo:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateLeagueSetup:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateChampionshipSetup:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateSaveLoad:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstatePauseFull:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstatePauseMinimal:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstatePauseNoSave:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case 21:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateMultiSetupOptions:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateFinalCeremony:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateGameIntro:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateTrainingEnd:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case kEstateGameClipsViewer:
                GameApp::InstantiateState();

                break;

            case kEstateAwards:
                GameApp::InstantiateState();

                break;

            case kEstateKeyConfig:
                GameApp::InstantiateState();

                break;

            case kEstateDromeExterior:
                switch (g_nPrevEngineState)
                {
                    case kEstateChampionshipSetup:
                    case kEstateAwards:
                    {
                        const auto nDifficulty = g_pGameEngine->GetDifficultyLevel();

                        switch (g_nPlayerMode)
                        {
                            case kPlayerModeExhibition:
                                RELEASE_ASSERT(0);
                                break;

                            case kPlayerModeTournament:
                                // Beating drome two on easy or drome eight on medium graduates the player.
                                if ((g_nDromeArena == 2 && nDifficulty == DifficultyLevel::kEasy) || (g_nDromeArena == 8 && nDifficulty == DifficultyLevel::kMedium))
                                {
                                    g_gameApp.m_nEngineState = kEstateTrainingEnd;
                                }

                                if (g_nDromeArena == -1)
                                {
                                    g_pGameEngine->m_pSaveObject->SetAllTeamsAvailable(1);
                                    g_gameApp.m_nEngineState = kEstateFinalCeremony;
                                }

                                break;
                        }

                        Screen::ClearBlack();
                        GameApp::InstantiateState();

                        break;
                    }

                    default:
                        if (g_bSessionActive == 0 && g_gameApp.FindCDRomDrive(0) == -1)
                        {
                            g_gameApp.m_nEngineState = kEstateMainMenu;
                        }
                        else
                        {
                            Screen::ClearBlack();
                            GameApp::InstantiateState();
                        }

                        break;
                }

                break;

            case kEstateLobby:
                g_nDromeTeamHome = g_pGameEngine->m_nTeamHomeIndex;
                g_nDromeTeamAway = g_pGameEngine->m_nTeamAwayIndex;
                g_nDromeArena    = g_pGameEngine->m_nArenaIndex;
                // Falls through

            case kEstateJumbotron:
                if (g_nPrevEngineState == kEstateInit)
                {
                    GameApp::InstantiateState();

                    break;
                }

                // Falls through

            case kEstateMainMenu:
                if (g_nPrevEngineState != kEstateMatchLive)
                {
                    Screen::ClearBlack();
                }

                GameApp::InstantiateState();

                break;

            case kEstateMatchLive:
            case 24:
                RELEASE_ASSERT(0);

                break;

            case kEstateMatchStart:
                g_pAudioMgr->FreeGroupBuffers(kSoundGroupPreload);
                g_pAudioMgr->StopEntry(kSoundSxxamba2);

                g_pGameEngine->SetTeams(static_cast<short>(g_nDromeTeamHome), static_cast<short>(g_nDromeTeamAway));
                g_pGameEngine->SetArena(static_cast<short>(g_nDromeArena));
                g_pGameEngine->RestartMatch();
                Screen::ClearBlack();

                if (g_bFullScreen != 0)
                {
                    Display::ChangeColorDepth(0);
                }

                g_pActiveScreen = nullptr;
                GameApp::SetCursorVisible(g_nSavedCursorVisible);
                g_pGameEngine->m_wRunState = kEngineRunPlaying;

                if (g_pActiveScreen != nullptr && g_pActiveScreen->HasPalette() != 0)
                {
                    Screen::ClearBlack();
                }

                if (g_hMdmTexDll && g_pColorPalette != nullptr)
                {
                    g_pColorPalette->SetEntries(0, 0x100, g_aSoftTexPalette);
                }

                Renderer::ClearFrame();
                Renderer::PresentFrame();

                if (Game::SetEngineState(kEstateMatchLive) != kEstateMatchLive)
                {
                    Game::SetEngineState(kEstateMatchLive);
                    g_pGameEngine->BeginMatchPlay();
                    g_pGameEngine->BroadcastEvent(kMsgStartDrag, 0, kSlotGod);
                }

                g_pAudioMgr->PlayCDTrack(kCDTrackRandom);
                g_pAudioMgr->PlaySpecific(kSoundSxxambb2);

                break;

            case kEstateQuit:
                Game::SetEngineState(kEstateLobby);
                pszText = g_gameApp.m_stringPool.LoadText(Math::Rand() % 3 + kStringReallyQuit);

                if (Window::AskYesNo(pszText,
                                     g_gameApp.m_stringPool.LoadText(kStringQuit)))
                {
                    Screen::ClearBlack();

                    // InitFrame runs before this check, so a run with no screen faults there.
                    if (g_pActiveScreen != nullptr)
                    {
                        g_pActiveScreen->InitFrame();
                    }

                    g_pActiveScreen = nullptr;
                    g_gameApp.OnShutdown();

                    break;
                }

                g_pActiveScreen->SetRepaintBackground(1);
                g_pActiveScreen->InitFrame();
                Game::SetEngineState(g_nPrevEngineState);

                break;
        }
    }
    else if (g_pActiveScreen != nullptr)
    {
        g_pActiveScreen->TickAnimations();
    }

    g_nPrevEngineState = g_gameApp.m_nEngineState;

    return 1;
}
