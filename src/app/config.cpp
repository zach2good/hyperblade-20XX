#include "common/compat.h"
#include "input/keycodes.h"
#include "globals.h"
#include "appglobals.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include "gameapp.h"
#include "common/fileobj.h"
#include "engine/messageids.h"
#include "graphics/graphicsglobals.h"
#include "engine/engineglobals.h"
#include "common/pathstring.h"
#include "common/commonglobals.h"
#include "common/staticinit.h"
#include "engine/cameraobject.h"
#include "engine/gameengine.h"
#include "graphics/renderer.h"
#include "app/window.h"
#include "graphics/gdi.h"
#include "common/string.h"
#include "ui/torchbitmaps.h"
#include "ui/buttonwidget.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "ui/screens/lobbyscreen.h"
#include "ui/pausemenu.h"
#include "ui/saveload.h"
#include "ui/screens/saveloaduiscreen.h"
#include "ui/widget.h"
#include "ui/screen.h"
#include "ui/screens/leaguesetupuiscreen.h"
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
#include "engine/messagequeue.h"
#include "input/keyboarddeviceobj.h"
#include "engine/objectclasses.h"
#include "engine/teamblock.h"
#include "engine/devicector.h"
#include "net/gamesession.h"
#include <stdarg.h>
#include <stdlib.h>
#include "ui/screens/championshipsetupuiscreen.h"
#include "graphics/viewextent.h"
#include "ui/screens/pausemenufullscreen.h"
#include "ui/screens/pausemenuminimalscreen.h"
#include "ui/screens/pausemenunosavescreen.h"
#include "ui/screens/multisetupoptionsscreen.h"
#include "ui/screens/gameintroscreen.h"
#include "ui/screens/trainingendscreen.h"
#include "engine/engineobject.h"
#include "graphics/colorpalette.h"
#include "frametimer.h"
#include <new.h>
#include "graphics/display.h"
#include "ui/infobarbase.h"
#include "graphics/cgpalette.h"
#include "input/input.h"
#include "app/wndproc.h"
#include "net/network.h"
#include "engine/game.h"

float g_flUnusedSteeringA  = g_flZeroSteering;
float g_flUnusedSteeringB  = g_flZeroSteering;
float g_flUnusedSteeringC  = g_flZeroSteering;
float g_flNetTracking      = g_flOneSteering;
float g_flBandwidth        = g_flOneSteering;
float g_flForceRelaxation  = g_flOneSteering;
float g_flMaxSpeed         = g_flOneSteering;
float g_flMaxAcceleration  = g_flOneSteering;
float g_flSpeedBurstFactor = g_flOneSteering;
float g_flStopForceFactor  = g_flOneSteering;
float g_flForceIncrement   = g_flOneSteering;
Vec3  g_vViewSteepness     = { g_flZeroSteering, g_flZeroSteering, g_flTwoSteering };
// The separator is doubled: VC6 turns the unrecognised escape into a bare D.
// STRING: HYPERX 0x004c4750
String g_strPrefs("Data\\DftKeybd.cfg");

// FUNCTION: HYPERX 0x0046d710
void Config::LoadPreferences(const char* pszFileName)
{
    char  szBuffer[MAX_PATH_LENGTH];
    FILE* pFile;

    // STRING: HYPERX 0x004bc990
    if ((pFile = fopen(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName), "r")) != 0)
    {
        if (fscanf(pFile, g_pszScanFormat, szBuffer) == 0)
        {
            fclose(pFile);

            // STRING: HYPERX 0x004c4de0
            g_gameApp.Fatal("Zero bytes read from %s!", String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData);
        }

        fclose(pFile);

        g_strPrefs.Assign(String::SafeStrlen(szBuffer), szBuffer);
    }
    else
    {
        // STRING: HYPERX 0x004c4da8
        g_gameApp.Error("Preferences file \"%s\" is missing or cannot be opened.", String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData);

        // STRING: HYPERX 0x004c4750
        g_strPrefs.Assign(String::SafeStrlen("Data\\DftKeybd.cfg"),
                          "Data\\DftKeybd.cfg");
    }

    Config::ParseFile(g_strPrefs.m_pchData, "Data\\DftKeybd.cfg", g_aKeyRows);
}

// A missing file still returns true.
// FUNCTION: HYPERX 0x0046d8b0
int Config::ParseFile(const char* pszData, const char*, KeyRow* pRows)
{
    FILE* const pFile = fopen(pszData, "r");

    if (pFile != 0)
    {
        for (int nRow = 0; nRow < 40; nRow++)
        {
            int nFlags;
            int nKey;
            int nIndex;

            // STRING: HYPERX 0x004c4e60
            fscanf(pFile, "%d %d %d: ", &nFlags, &nKey, &nIndex);

            pRows[nRow].cFlags = (unsigned char)nFlags;
            pRows[nRow].nKey   = (unsigned short)nKey;
            pRows[nRow].nIndex = (unsigned short)nIndex;
        }

        // STRING: HYPERX 0x004c4e50
        fscanf(pFile, " %f %f %f %f", &g_flForceIncrement, &g_flMaxAcceleration, &g_flMaxSpeed, &g_flForceRelaxation);

        int nVictimCam;

        // STRING: HYPERX 0x004c4e34
        fscanf(pFile, "  @v02 %d %f %f %f %f %f :", &nVictimCam, &g_flBandwidth, &g_flNetTracking, &g_flStopForceFactor, &g_flSpeedBurstFactor, &g_vViewSteepness.z);

        g_bVictimCam = (unsigned char)nVictimCam;

        // STRING: HYPERX 0x004c4d9c
        fscanf(pFile, " %f %f %f :", &g_vBirdsEyeTrackOffset.x, &g_vBirdsEyeTrackOffset.y, &g_vBirdsEyeTrackOffset.z);

        fscanf(pFile, " %f %f %f :", &g_vFirstPersonTrackOffset.x, &g_vFirstPersonTrackOffset.y, &g_vFirstPersonTrackOffset.z);

        fscanf(pFile, " %f %f %f :", &g_vOverShoulderTrackOffset.x, &g_vOverShoulderTrackOffset.y, &g_vOverShoulderTrackOffset.z);

        fscanf(pFile, " %f %f %f :", &g_vOverShoulderRokTrackOffset.x, &g_vOverShoulderRokTrackOffset.y, &g_vOverShoulderRokTrackOffset.z);

        fclose(pFile);

        return 1;
    }

    // STRING: HYPERX 0x004c4dfc
    g_gameApp.Fatal("Configuration file \"%s\" is missing or cannot be opened.", pszData);

    return 1;
}

// Message table entries the pause key shows, and how long they stay up.
#define MSG_PAUSED    0x4f
#define MSG_TOGGLE_MS 0x5dc

// FUNCTION: HYPERX 0x0046d9f0
void GameApp::HandleGlobalKeyEvent(int nKey)
{
    if (g_pRenderer != 0 && g_pRenderer->m_bQuitPromptVisible != 0)
    {
        switch (nKey)
        {
            case Key::RETURN:
                g_gameApp.m_nEngineState = ESTATE_LOBBY;

                g_pGameEngine->m_bMatchOver = 1;

                g_pGameEngine->ResetPeriodToFirst();

                if (g_bSessionActive != 0 && g_pGameSession != 0)
                {
                    GameSession::Disconnect();
                }

                GameApp::ResetToSinglePlayer();

                // Falls into the escape case: both keys end in the same call.
            case Key::ESCAPE:
                g_pRenderer->HideQuitPrompt();
                break;
        }

        return;
    }

    switch (nKey)
    {
        case Key::PAUSE:
            if (g_bSessionActive == 0 && g_pGameEngine != 0 && g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE && g_pRenderer != 0 && g_pRenderer->m_bQuitPromptVisible == 0)
            {
                if ((short)g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING)
                {
                    g_pMessageQueue->ProcessAndClear();
                    g_pMessageQueue->PushFromTable(MSG_PAUSED, 0, MSG_TOGGLE_MS);

                    Game::RunFrame();

                    g_bSingleStepActive = 1;

                    g_pGameEngine->SetRunState(0, 0);
                    return;
                }

                if (g_bSingleStepActive != 0)
                {
                    g_pMessageQueue->ProcessAndClear();

                    g_bSingleStepActive = 0;

                    g_pGameEngine->SetRunState(1, 0);
                    return;
                }
            }
            break;

        case Key::TAB:
            g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->PostSceneEvent(MSG_MOVE_EYE_TRACK, 5);

            g_nTabCycle = (short)((g_nTabCycle + 1) % 2);
            break;

        case Key::ESCAPE:
            if (g_pRenderer != 0 && g_pGameEngine != 0 && g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE && (short)g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING)
            {
                g_pRenderer->ShowQuitPrompt();
                return;
            }
            break;

        case Key::F9:
        {
            const short nGoalieSlot = (g_pGameEngine->m_nViewedSlot & 1) + GOALIE_SLOT_A;
            g_pGameEngine->GetSlot(nGoalieSlot)->PostSceneEvent(MSG_ATTACK, 0);
        }
        break;

        case Key::F10:
        {
            const short nGoalieSlot = (g_pGameEngine->m_nViewedSlot & 1) + GOALIE_SLOT_A;
            g_pGameEngine->GetSlot(nGoalieSlot)->PostSceneEvent(MSG_GOAL_SCORED, 0);
        }
        break;
    }
}

// FUNCTION: HYPERX 0x00480c60
int Config::IsCommentLine(const char* pszLine)
{
    char cFirst;

    if (pszLine != 0)
    {
        if (sscanf(pszLine, g_pszCharFormat, &cFirst) == 1)
        {
            if (cFirst == '#')
            {
                return 1;
            }
        }
    }

    return 0;
}

// The physics sheet's "Energy On" box.
unsigned char g_bEnergyOn = 1;

// Forty rows of flags, key and index. The 284 bytes after it hold unread rows of the same shape.
short g_aDefaultKeyBindings[120] = {
    1, 38, 0, 1, 40, 1, 1, 37, 2, 1, 39, 3, 5, 37, 2, 5, 39, 3, 5, 38, 0, 1, 13, 4, 1, 32, 7, 9, 37, 2, 9, 39, 3, 9, 38, 0, 1, 0, 58, 1, 0, 58, 1, 0, 58, 1, 0, 58, 1, 81, 36, 1, 88, 42, 5, 88, 42, 1, 89, 43, 5, 89, 43, 1, 90, 44, 5, 90, 44, 1, 9, 6, 1, 65, 22, 1, 27, 5, 1, 2, 9, 1, 69, 26, 1, 70, 27, 1, 113, 11, 1, 115, 13, 1, 118, 16, 1, 119, 17, 1, 13, 4, 1, 32, 7, 1, 67, 24, 1, 79, 32, 5, 13, 4, 1, 66, 23, 5, 66, 23
};

short g_nTabCycle = 1;
