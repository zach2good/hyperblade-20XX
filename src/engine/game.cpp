// The game at the top level: engine setup and teardown, one frame, and the state machine.

#include "common/compat.h"
#include <SDL3/SDL.h>
#include "graphics/hudlayer.h"
#include "globals.h"
#include <stdlib.h>
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
#include "app/win32.h"
#include "common/stub.h"
#include "app/stringids.h"
#include "sound/soundids.h"
#include "engine/messageids.h"
#include "engine/objectclasses.h"

float g_flUnusedGame                = g_flZeroGame;
Vec3  g_vBirdsEyeTrackOffset        = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vFirstPersonTrackOffset     = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vOverShoulderTrackOffset    = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vOverShoulderRokTrackOffset = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vFallTrackOffset            = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vNoOffset                   = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };
Vec3  g_vZeroMarker                 = { g_flZeroPlanArg, g_flZeroPlanArg, g_flZeroPlanArg };

// FUNCTION: HYPERX 0x0044c1b0
void Game::RunFrame(void)
{
    Game::Tick();
    Game::Draw(1.0f);
}

void Game::Tick()
{
    if (g_frameTimer.bFrameTimesOn)
    {
        g_frameTimer.nClockReads++;
        g_dwFrameGameLogicStart        = (unsigned int)SDL_GetTicks();
        g_frameTimer.dwFrameStartTicks = g_dwFrameGameLogicStart;
    }
    else
    {
        g_dwFrameGameLogicStart = (unsigned int)SDL_GetTicks();
    }

    if (g_pRenderer != 0)
    {
        g_pRenderer->BeginTick();
    }

    FramePacing::BeginTick();

    g_pGameEngine->PerFrameUpdate();

    if (g_frameTimer.bFrameTimesOn != 0)
    {
        g_frameTimer.nPerfSampleCount++;
        g_frameTimer.dAccGameLogicMs += (double)((unsigned int)SDL_GetTicks() - g_frameTimer.dwFrameStartTicks);
    }
}

void Game::Draw(float flAlpha)
{
    FramePacing::DrawViews(flAlpha);

    if (g_pRenderer != 0)
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
        g_frameTimer.dAccRenderMs += (double)((unsigned int)SDL_GetTicks() - g_frameTimer.dwFrameStartTicks);
    }

    g_frameTimer.nFrames++;
}

// Builds the game's owners in the original's order, timing each stage.
// FUNCTION: HYPERX 0x0045c700
void Game::InitializeEngine()
{
    int nStart = GameApp::GetInstallLevel();

    // A new of a type with a constructor, for the null test and exception frame.
    g_pTexManager = new CD3DTextureManager;

    int nDelta = GameApp::GetInstallLevel() - nStart;

    g_pTexManager->m_nInstallLevel += nDelta;

    g_pGameOptions = new GameOptions;

    nStart = GameApp::GetInstallLevel();

    g_pGameEngine = new GameEngine;

    nDelta = GameApp::GetInstallLevel() - nStart;

    g_pGameEngine->m_nLoadCost += nDelta;

    nStart = GameApp::GetInstallLevel();

    g_pAudioMgr = new AudioMgr;

    g_pAudioMgr->Init();

    nDelta = GameApp::GetInstallLevel() - nStart;

    g_pAudioMgr->nInitTimeMs += nDelta;

    g_pGameEngine->Reset();

    Game::InitMatchMarkers();

    nStart = GameApp::GetInstallLevel();

    AnimRegistry* const pAnimDb = new AnimRegistry;

    g_pAnimRegistry = pAnimDb;

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

    g_pMessageQueue = new MessageQueue;

    // STRING: HYPERX 0x004c41a4
    g_pMessageQueue->LoadList("infobtxt.lst");
}

// The renderer is not nulled, so a second call would tear it down again.
// FUNCTION: HYPERX 0x0045c9c0
void Game::ShutdownEngine(void)
{
    if (g_pMessageQueue != 0)
    {
        g_pMessageQueue = g_pMessageQueue->FreeAll();
    }

    delete g_pRenderer;

    if (g_pAnimRegistry != 0)
    {
        delete g_pAnimRegistry;

        g_pAnimRegistry = 0;
    }

    if (g_pAudioMgr != 0)
    {
        delete g_pAudioMgr;

        g_pAudioMgr = 0;
    }

    if (g_pGameEngine != 0)
    {
        delete g_pGameEngine;

        g_pGameEngine = 0;
    }

    if (g_pGameOptions != 0)
    {
        delete g_pGameOptions;

        g_pGameOptions = 0;
    }

    if (g_pTexManager != 0)
    {
        delete g_pTexManager;

        g_pTexManager = 0;
    }
}

// Eight markers against the arena ellipsoid; the y-minus one asks for its height at y = +6, not -6.
// FUNCTION: HYPERX 0x0047d1c0
void Game::InitMatchMarkers(void)
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
    const unsigned int previous = g_gameApp.m_nEngineState;

    g_gameApp.m_nEngineState = state;

    return previous;
}

// Runs on the frame the state changed; every other frame only ticks the current screen.
// FUNCTION: HYPERX 0x00491de0
unsigned int Game::UpdateState(void)
{
    char* pszText;

    if (g_gameApp.m_nEngineState != (unsigned int)g_nPrevEngineState)
    {
        if (g_gameApp.m_nEngineState != ESTATE_MATCH_START)
        {
            g_pAudioMgr->PreloadGroup(SOUND_GROUP_PRELOAD);
        }

        if (g_nPrevEngineState == ESTATE_INIT || g_nPrevEngineState == ESTATE_MATCH_LIVE)
        {
            // Reached with the manager in ECX, which the function ignores.
            g_pAudioMgr->StopAllSfx();
            g_pAudioMgr->PlaySpecific(SOUND_SXXAMBA2);

            if (g_pGameOptions != 0 && g_pGameOptions->m_nInstallLevel != 0)
            {
                g_pAudioMgr->PlayCDTrack(CD_TRACK_MENU);
            }
            else
            {
                g_pAudioMgr->StopCD();
            }

            if (g_nPrevEngineState == ESTATE_MATCH_LIVE)
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
            case ESTATE_GAME_CONFIG:
                if (g_nPrevEngineState != ESTATE_KEY_CONFIG)
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

            case ESTATE_TOURNAMENT_SETUP:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_HEAD_TO_HEAD_SETUP:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_MATCH_INFO:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_LEAGUE_SETUP:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_CHAMPIONSHIP_SETUP:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_SAVE_LOAD:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_PAUSE_FULL:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_PAUSE_MINIMAL:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_PAUSE_NO_SAVE:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case 21:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_MULTI_SETUP_OPTIONS:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_FINAL_CEREMONY:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_GAME_INTRO:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_TRAINING_END:
                Screen::ClearBlack();
                GameApp::InstantiateState();

                break;

            case ESTATE_GAME_CLIPS_VIEWER:
                GameApp::InstantiateState();

                break;

            case ESTATE_AWARDS:
                GameApp::InstantiateState();

                break;

            case ESTATE_KEY_CONFIG:
                GameApp::InstantiateState();

                break;

            case ESTATE_DROME_EXTERIOR:
                switch (g_nPrevEngineState)
                {
                    case ESTATE_CHAMPIONSHIP_SETUP:
                    case ESTATE_AWARDS:
                    {
                        const int nDifficulty = g_pGameEngine->GetDifficultyLevel();

                        switch (g_nPlayerMode)
                        {
                            case PLAYER_MODE_EXHIBITION:
                                RELEASE_ASSERT(0);
                                break;

                            case PLAYER_MODE_TOURNAMENT:
                                // Beating drome two on easy or drome eight on medium graduates the player.
                                if ((g_nDromeArena == 2 && nDifficulty == 0) || (g_nDromeArena == 8 && nDifficulty == 1))
                                {
                                    g_gameApp.m_nEngineState = ESTATE_TRAINING_END;
                                }

                                if (g_nDromeArena == -1)
                                {
                                    g_pGameEngine->m_pSaveObject->SetAllTeamsAvailable(1);
                                    g_gameApp.m_nEngineState = ESTATE_FINAL_CEREMONY;
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
                            g_gameApp.m_nEngineState = ESTATE_MAIN_MENU;
                        }
                        else
                        {
                            Screen::ClearBlack();
                            GameApp::InstantiateState();
                        }

                        break;
                }

                break;

            case ESTATE_LOBBY:
                g_nDromeTeamHome = g_pGameEngine->m_nTeamHomeIndex;
                g_nDromeTeamAway = g_pGameEngine->m_nTeamAwayIndex;
                g_nDromeArena    = g_pGameEngine->m_nArenaIndex;
                // Falls through

            case ESTATE_JUMBOTRON:
                if (g_nPrevEngineState == ESTATE_INIT)
                {
                    GameApp::InstantiateState();

                    break;
                }

                // Falls through

            case ESTATE_MAIN_MENU:
                if (g_nPrevEngineState != ESTATE_MATCH_LIVE)
                {
                    Screen::ClearBlack();
                }

                GameApp::InstantiateState();

                break;

            case ESTATE_MATCH_LIVE:
            case 24:
                RELEASE_ASSERT(0);

                break;

            case ESTATE_MATCH_START:
                g_pAudioMgr->FreeGroupBuffers(SOUND_GROUP_PRELOAD);
                g_pAudioMgr->StopEntry(SOUND_SXXAMBA2);

                g_pGameEngine->SetTeams((short)g_nDromeTeamHome, (short)g_nDromeTeamAway);
                g_pGameEngine->SetArena((short)g_nDromeArena);
                g_pGameEngine->RestartMatch();
                Screen::ClearBlack();

                if (g_bFullScreen != 0)
                {
                    Display::ChangeColorDepth(0);
                }

                delete g_pActiveScreen;

                g_pActiveScreen = 0;
                GameApp::SetCursorVisible(g_nSavedCursorVisible);
                g_pGameEngine->m_wRunState = ENGINE_RUN_PLAYING;

                if (g_pActiveScreen != 0 && g_pActiveScreen->HasPalette() != 0)
                {
                    Screen::ClearBlack();
                }

                if (g_hMdmTexDll != 0 && g_pColorPalette != 0)
                {
                    g_pColorPalette->SetEntries(0, 0x100, g_aSoftTexPalette);
                }

                Renderer::ClearFrame();
                Renderer::PresentFrame();

                if (Game::SetEngineState(ESTATE_MATCH_LIVE) != ESTATE_MATCH_LIVE)
                {
                    Game::SetEngineState(ESTATE_MATCH_LIVE);
                    g_pGameEngine->BeginMatchPlay();
                    g_pGameEngine->BroadcastEvent(MSG_START_DRAG, 0, SLOT_GOD);
                }

                g_pAudioMgr->PlayCDTrack(CD_TRACK_RANDOM);
                g_pAudioMgr->PlaySpecific(SOUND_SXXAMBB2);

                break;

            case ESTATE_QUIT:
                Game::SetEngineState(ESTATE_LOBBY);
                pszText = g_gameApp.m_stringPool.LoadText(rand() % 3 + STRING_REALLY_QUIT);

                if (Win32::AskYesNo(pszText,
                                    g_gameApp.m_stringPool.LoadText(STRING_QUIT)))
                {
                    Screen::ClearBlack();

                    // InitFrame runs before this check, so a run with no screen faults there.
                    if (g_pActiveScreen != 0)
                    {
                        g_pActiveScreen->InitFrame();
                    }

                    delete g_pActiveScreen;

                    g_pActiveScreen = 0;
                    g_gameApp.OnShutdown();

                    break;
                }

                g_pActiveScreen->SetRepaintBackground(1);
                g_pActiveScreen->InitFrame();
                Game::SetEngineState(g_nPrevEngineState);

                break;
        }
    }
    else if (g_pActiveScreen != 0)
    {
        g_pActiveScreen->TickAnimations();
    }

    g_nPrevEngineState = g_gameApp.m_nEngineState;

    return 1;
}
