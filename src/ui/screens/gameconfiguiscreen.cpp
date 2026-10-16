#include "common/compat.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "graphics/gdi.h"
class GameConfigUIScreen;
#include "ui/torchbitmaps.h"
#include "ui/widget.h"
#include "ui/screen.h"
#include "ui/uiglobals.h"
#include <string.h>
#include <stdio.h>
#include "ui/screens/gameconfiguiscreen.h"
#include "sound/audiomgr.h"
#include "ui/radar.h"
#include "common/stringpool.h"
#include "app/gameapp.h"
#include "ui/loopflipbookwidget.h"
#include "ui/flipbookwidget.h"
#include "ui/displaywidget.h"
#include "ui/saveobject.h"
#include "engine/game.h"
#include "common/stub.h"
#include "sound/soundids.h"
#include "app/stringids.h"

// Settings are copied twenty bytes below the live block, so cancelling can restore them.

// The three period lengths the options offer, in milliseconds.
enum PeriodLength
{
    PERIOD_LENGTH_SHORT  = 180000,
    PERIOD_LENGTH_MEDIUM = 300000,
    PERIOD_LENGTH_LONG   = 420000,
};

// FUNCTION: HYPERX 0x00479ad0
GameConfigUIScreen::~GameConfigUIScreen()
{
}

// FUNCTION: HYPERX 0x00479b00
GameConfigUIScreen::GameConfigUIScreen()
{
    m_nLastWidget = GAME_CONFIG_DIFFICULTY_RIGHT;

    if (g_nPrevEngineState == ESTATE_LOBBY || g_nPrevEngineState == ESTATE_PAUSE_FULL || g_nPrevEngineState == ESTATE_PAUSE_NO_SAVE)
    {
        g_nOptionAttackMode = g_nOptionsSavedAttackMode = g_pGameEngine->m_nAttackMode;
        g_nOptionTeammateMode = g_nOptionsSavedTeammateMode = g_pGameEngine->m_nTeammateMode;
        g_nPeriodLengthMs = g_nOptionsSavedPeriodLengthMs = g_pGameEngine->m_nPeriodLengthMs;

        g_nOptionSfxVolume = g_nOptionsSavedSfxVolume = g_pAudioMgr->SfxReadVolume();
        g_nOptionMusicVolume = g_nOptionsSavedCdVolume = g_pAudioMgr->CDReadVolume();

        g_nOptionBrutality = g_nOptionsSavedBrutality = g_pGameEngine->m_nBrutality;
        g_nOptionDifficulty = g_nOptionsSavedDifficulty = g_pGameEngine->GetDifficultyLevel();

        if (g_nPeriodLengthMs == PERIOD_LENGTH_SHORT)
        {
            g_nOptionPeriodLength = 1;
        }
        else if (g_nPeriodLengthMs == PERIOD_LENGTH_MEDIUM)
        {
            g_nOptionPeriodLength = 2;
        }
        else if (g_nPeriodLengthMs == PERIOD_LENGTH_LONG)
        {
            g_nOptionPeriodLength = 3;
        }
        else
        {
            g_nOptionPeriodLength = 0;
        }
    }

    SetupWidgets();
}

// FUNCTION: HYPERX 0x00479c40
void GameConfigUIScreen::SetupWidgets()
{
    BITMAP bm;
    int    nOption;

    // STRING: HYPERX 0x004c2eb8
    m_hBackground = GDI::LoadBitmapAsBackground("Hviewl", &m_hPalette);

    m_apWidgets[UI_WIDGET_TAB_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 4]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_CONFIRM_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_CONFIRM_PLATE]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_CONFIRM_PLATE]->SetDepressed(1);

    m_apWidgets[UI_WIDGET_OKAY]->ClearFrames();
    // STRING: HYPERX 0x004c2168
    m_apWidgets[UI_WIDGET_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okay"));
    // STRING: HYPERX 0x004c2160
    m_apWidgets[UI_WIDGET_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okayr"));
    // STRING: HYPERX 0x004c2158
    m_apWidgets[UI_WIDGET_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okayd"));

    m_apWidgets[UI_WIDGET_CANCEL]->ClearFrames();
    // STRING: HYPERX 0x004c2150
    m_apWidgets[UI_WIDGET_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Cancl"));
    // STRING: HYPERX 0x004c2148
    m_apWidgets[UI_WIDGET_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Canclr"));
    // STRING: HYPERX 0x004c2140
    m_apWidgets[UI_WIDGET_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Cancld"));

    // STRING: HYPERX 0x004c6718
    HBITMAP hBitmap = GDI::LoadBitmapHandle("Optbkgd");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    Widget* pWidget = AppendWidget(new LoopFlipbookWidget(0, 0, 0, 0, 0, 1, 0, 0, 0, 0, this, 0, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->InitAfterLoad();

    for (nOption = 0; nOption < GAME_CONFIG_BUTTON_COUNT; nOption++)
    {
        hBitmap = GDI::LoadBitmapHandle(g_aConfigButtonArt[nOption][0]);
        GDI::GetBitmapDimensions(hBitmap, &bm);
        pWidget = AppendWidget(new FlipbookWidget(g_aConfigButtonPos[nOption].nX,
                                                  g_aConfigButtonPos[nOption].nY,
                                                  bm.bmWidth,
                                                  bm.bmHeight,
                                                  0,
                                                  0,
                                                  2,
                                                  0,
                                                  1,
                                                  2,
                                                  this,
                                                  -1,
                                                  -1,
                                                  -1));
        pWidget->AppendFrame(hBitmap);
        pWidget->AppendFrame(GDI::LoadBitmapHandle(g_aConfigButtonArt[nOption][1]));
        pWidget->AppendFrame(GDI::LoadBitmapHandle(g_aConfigButtonArt[nOption][2]));
    }

    for (nOption = 0; nOption < GAME_CONFIG_ROW_COUNT; nOption++)
    {
        hBitmap = GDI::LoadBitmapHandle(GetOptionBitmapName(nOption + GAME_CONFIG_ATTACK_MODE));
        GDI::GetBitmapDimensions(hBitmap, &bm);
        pWidget = AppendWidget(new DisplayWidget(g_aConfigDisplayPos[nOption].nX,
                                                 g_aConfigDisplayPos[nOption].nY,
                                                 0,
                                                 0,
                                                 this,
                                                 -1));
        pWidget->AppendFrame(hBitmap);
    }

    for (nOption = 0; nOption < GAME_CONFIG_ARROW_COUNT; nOption++)
    {
        const int nSide = nOption % 2;

        if (nSide != 0)
        {
            hBitmap = GDI::LoadBitmapHandle(g_pszOptionArrowRight1);
        }
        else
        {
            hBitmap = GDI::LoadBitmapHandle(g_pszOptionArrowLeft1);
        }

        GDI::GetBitmapDimensions(hBitmap, &bm);
        pWidget = AppendWidget(new FlipbookWidget(g_aConfigArrowPos[nOption].nX,
                                                  g_aConfigArrowPos[nOption].nY,
                                                  bm.bmWidth,
                                                  bm.bmHeight,
                                                  0,
                                                  0,
                                                  2,
                                                  0,
                                                  1,
                                                  2,
                                                  this,
                                                  -1,
                                                  -1,
                                                  SOUND_SXBUTT1));
        pWidget->AppendFrame(hBitmap);

        if (nSide != 0)
        {
            pWidget->AppendFrame(GDI::LoadBitmapHandle(g_pszOptionArrowRight3));
            pWidget->AppendFrame(GDI::LoadBitmapHandle(g_pszOptionArrowRight2));
        }
        else
        {
            pWidget->AppendFrame(GDI::LoadBitmapHandle(g_pszOptionArrowLeft3));
            pWidget->AppendFrame(GDI::LoadBitmapHandle(g_pszOptionArrowLeft2));
        }
    }

    InitAllWidgets();

    if (g_nPrevEngineState != ESTATE_LOBBY && m_nReturnState != ESTATE_LOBBY && g_pGameEngine->m_pSaveObject->GetBracketPos() != 0)
    {
        m_apWidgets[GAME_CONFIG_DIFFICULTY_LEFT]->SetHidden(1);
        m_apWidgets[GAME_CONFIG_DIFFICULTY_RIGHT]->SetHidden(1);
    }
}

#define OPTIONS_MESSAGE_MAX 852

#define VOLUME_MAX 7

// FUNCTION: HYPERX 0x0047a160
void GameConfigUIScreen::OnOtherButton(int nMsg, unsigned int nId)
{
    char      szMessage[OPTIONS_MESSAGE_MAX];
    int       nValue;
    HINSTANCE hInstance;

    if (nMsg != WM_LBUTTONUP)
    {
        return;
    }

    switch (nId)
    {
        case GAME_CONFIG_CUSTOMIZE_KEYS:
            Game::SetEngineState(ESTATE_KEY_CONFIG);
            return;

        case GAME_CONFIG_JOYSTICK_INFO:
            hInstance = g_gameApp.m_hInstance;
            strcpy(szMessage, g_gameApp.m_stringPool.LoadText(STRING_JOYSTICK_CONTROL_PANEL));
            hInstance = g_gameApp.m_hInstance;
            strcat(szMessage, g_gameApp.m_stringPool.LoadText(STRING_GAME_PAD_HELP));
            GameApp::ShowMessageBox(g_gameApp.m_hWnd, szMessage, g_gameApp.m_strCaption.m_pchData, MB_OK);
            return;

        case GAME_CONFIG_ATTACK_MODE_LEFT:
            nValue = g_nOptionAttackMode;

            if (nValue != 0)
            {
                g_nOptionAttackMode = nValue - 1;
                LoadOptionBitmap(GAME_CONFIG_ATTACK_MODE);

                return;
            }

            g_nOptionAttackMode = 1;
            LoadOptionBitmap(GAME_CONFIG_ATTACK_MODE);

            return;

        case GAME_CONFIG_ATTACK_MODE_RIGHT:
            nValue = g_nOptionAttackMode;

            if (nValue < 1)
            {
                g_nOptionAttackMode = nValue + 1;
                LoadOptionBitmap(GAME_CONFIG_ATTACK_MODE);

                return;
            }

            g_nOptionAttackMode = 0;
            LoadOptionBitmap(GAME_CONFIG_ATTACK_MODE);

            return;

        case GAME_CONFIG_TEAMMATE_BIAS_LEFT:
            nValue = g_nOptionTeammateMode;

            if (nValue != 0)
            {
                g_nOptionTeammateMode = nValue - 1;
                LoadOptionBitmap(GAME_CONFIG_TEAMMATE_BIAS);

                return;
            }

            g_nOptionTeammateMode = 1;
            LoadOptionBitmap(GAME_CONFIG_TEAMMATE_BIAS);

            return;

        case GAME_CONFIG_TEAMMATE_BIAS_RIGHT:
            nValue = g_nOptionTeammateMode;

            if (nValue < 1)
            {
                g_nOptionTeammateMode = nValue + 1;
                LoadOptionBitmap(GAME_CONFIG_TEAMMATE_BIAS);

                return;
            }

            g_nOptionTeammateMode = 0;
            LoadOptionBitmap(GAME_CONFIG_TEAMMATE_BIAS);

            return;

        case GAME_CONFIG_PERIOD_LENGTH_LEFT:
            g_nOptionPeriodLength--;

            if (g_nOptionPeriodLength < 1)
            {
                g_nOptionPeriodLength = 3;
            }

            LoadOptionBitmap(GAME_CONFIG_PERIOD_LENGTH);

            return;

        case GAME_CONFIG_PERIOD_LENGTH_RIGHT:
            g_nOptionPeriodLength++;

            if (g_nOptionPeriodLength > 3)
            {
                g_nOptionPeriodLength = 1;
            }

            LoadOptionBitmap(GAME_CONFIG_PERIOD_LENGTH);

            return;

        case GAME_CONFIG_MUSIC_VOLUME_LEFT:
            g_nOptionMusicVolume--;

            if (g_nOptionMusicVolume < 0)
            {
                g_nOptionMusicVolume = 0;
            }

            g_pAudioMgr->CDFadeVolume(g_nOptionMusicVolume);
            g_nOptionMusicVolume = g_pAudioMgr->CDReadVolume();
            LoadOptionBitmap(GAME_CONFIG_MUSIC_VOLUME);

            return;

        case GAME_CONFIG_MUSIC_VOLUME_RIGHT:
            g_nOptionMusicVolume++;

            if (g_nOptionMusicVolume >= VOLUME_MAX + 1)
            {
                g_nOptionMusicVolume = VOLUME_MAX;
            }

            g_pAudioMgr->CDFadeVolume(g_nOptionMusicVolume);
            g_nOptionMusicVolume = g_pAudioMgr->CDReadVolume();
            LoadOptionBitmap(GAME_CONFIG_MUSIC_VOLUME);

            return;

        case GAME_CONFIG_SFX_VOLUME_LEFT:
            g_nOptionSfxVolume--;

            if (g_nOptionSfxVolume < 0)
            {
                g_nOptionSfxVolume = 0;
            }

            g_pAudioMgr->SfxFadeVolume(g_nOptionSfxVolume);
            g_nOptionSfxVolume = g_pAudioMgr->SfxReadVolume();
            LoadOptionBitmap(GAME_CONFIG_SFX_VOLUME);

            return;

        case GAME_CONFIG_SFX_VOLUME_RIGHT:
            g_nOptionSfxVolume++;

            if (g_nOptionSfxVolume >= VOLUME_MAX + 1)
            {
                g_nOptionSfxVolume = VOLUME_MAX;
            }

            g_pAudioMgr->SfxFadeVolume(g_nOptionSfxVolume);
            g_nOptionSfxVolume = g_pAudioMgr->SfxReadVolume();
            LoadOptionBitmap(GAME_CONFIG_SFX_VOLUME);

            return;

        case GAME_CONFIG_BRUTALITY_LEFT:
            g_nOptionBrutality--;

            if (g_nOptionBrutality < 0)
            {
                g_nOptionBrutality = 1;
            }

            LoadOptionBitmap(GAME_CONFIG_BRUTALITY);

            return;

        case GAME_CONFIG_BRUTALITY_RIGHT:
            g_nOptionBrutality++;

            if (g_nOptionBrutality > 1)
            {
                g_nOptionBrutality = 0;
            }

            LoadOptionBitmap(GAME_CONFIG_BRUTALITY);

            return;

        case GAME_CONFIG_DIFFICULTY_LEFT:
            g_nOptionDifficulty--;

            if (g_nOptionDifficulty < 0)
            {
                g_nOptionDifficulty = 2;
            }

            LoadOptionBitmap(GAME_CONFIG_DIFFICULTY);

            return;

        case GAME_CONFIG_DIFFICULTY_RIGHT:
            g_nOptionDifficulty++;

            if (g_nOptionDifficulty > 2)
            {
                g_nOptionDifficulty = 0;
            }

            LoadOptionBitmap(GAME_CONFIG_DIFFICULTY);

            return;

        case WIDGET_ID_MOUSE:
        case WIDGET_ID_RETURN:
            return;

        default:
            RELEASE_ASSERT(0);
            return;
    }
}

// The difficulty setter's answer is discarded.

// FUNCTION: HYPERX 0x0047a5e0
void GameConfigUIScreen::OnOkayClick()
{
    g_pGameEngine->SetAttackMode(g_nOptionAttackMode);
    g_pGameEngine->SetTeammateMode(g_nOptionTeammateMode);

    g_pGameEngine->SetPeriodLengthMs(g_nPeriodLengthMs);
    g_pGameEngine->SetBrutality(g_nOptionBrutality);
    g_pGameEngine->SetDifficultyLevel(g_nOptionDifficulty);

    Game::SetEngineState(m_nReturnState);
}

// Only the volumes are undone; nothing else is written until OK.

// FUNCTION: HYPERX 0x0047a650
void GameConfigUIScreen::OnCancelClick()
{
    g_pAudioMgr->SfxFadeVolume(g_nOptionsSavedSfxVolume);
    g_pAudioMgr->CDFadeVolume(g_nOptionsSavedCdVolume);

    Game::SetEngineState(m_nReturnState);
}

// Also sets the period length: 180000, 300000 or 420000 ms.
// FUNCTION: HYPERX 0x0047a690
const char* GameConfigUIScreen::GetOptionBitmapName(int nId)
{
    int nLevel;

    switch (nId)
    {
        case GAME_CONFIG_ATTACK_MODE:
            return g_apAttackModeBitmapNames[g_nOptionAttackMode];

        case GAME_CONFIG_TEAMMATE_BIAS:
            return g_apTeammateModeBitmapNames[g_nOptionTeammateMode];

        case GAME_CONFIG_PERIOD_LENGTH:
            if (g_nOptionPeriodLength == 1)
            {
                g_nPeriodLengthMs = PERIOD_LENGTH_SHORT;
            }
            else if (g_nOptionPeriodLength == 2)
            {
                g_nPeriodLengthMs = PERIOD_LENGTH_MEDIUM;

                // STRING: HYPERX 0x004c675c
                return "perleng5";
            }
            else if (g_nOptionPeriodLength == 3)
            {
                g_nPeriodLengthMs = PERIOD_LENGTH_LONG;

                // STRING: HYPERX 0x004c6750
                return "perleng7";
            }

            // STRING: HYPERX 0x004c6768
            return "perleng3";

        case GAME_CONFIG_MUSIC_VOLUME:
            nLevel = g_nOptionMusicVolume;

            if (nLevel < 0)
            {
                nLevel = 0;
            }
            else if (nLevel >= 8)
            {
                nLevel = 7;
            }

            // STRING: HYPERX 0x004c6748
            sprintf(g_szMusicBarName, "opbar%d", nLevel + 1);

            return g_szMusicBarName;

        case GAME_CONFIG_SFX_VOLUME:
            nLevel = g_nOptionSfxVolume;

            if (nLevel < 0)
            {
                nLevel = 0;
            }
            else if (nLevel >= 8)
            {
                nLevel = 7;
            }

            sprintf(g_szSfxBarName, "opbar%d", nLevel + 1);

            return g_szSfxBarName;

        case GAME_CONFIG_BRUTALITY:
            if (g_nOptionBrutality == 0)
            {
                // STRING: HYPERX 0x004c6740
                return "brutal1";
            }

            if (g_nOptionBrutality == 1)
            {
                // STRING: HYPERX 0x004c6738
                return "brutal2";
            }
            break;

        case GAME_CONFIG_DIFFICULTY:
            if (g_nOptionDifficulty == 0)
            {
                // STRING: HYPERX 0x004c6730
                return "diffic1";
            }

            if (g_nOptionDifficulty == 1)
            {
                // STRING: HYPERX 0x004c6728
                return "diffic2";
            }

            if (g_nOptionDifficulty == 2)
            {
                // STRING: HYPERX 0x004c6720
                return "diffic3";
            }
            break;
    }

    // STRING: HYPERX 0x004c6490
    return "opbar0";
}

// FUNCTION: HYPERX 0x0047a7f0
void GameConfigUIScreen::LoadOptionBitmap(int nOption)
{
    m_apWidgets[nOption]->ClearFrames();

    const HBITMAP hBitmap = GDI::LoadBitmapHandle(GetOptionBitmapName(nOption));

    m_apWidgets[nOption]->AppendFrame(hBitmap);
    m_apWidgets[nOption]->SetDirtyFlag(1);
}

// clang-format off
// Each string marker has to stay on its own line above its literal.
const char* g_aConfigButtonArt[2][3] = {
    {
        // STRING: HYPERX 0x004c6710
        "cuskey",
        // STRING: HYPERX 0x004c6708
        "cuskeyr",
        // STRING: HYPERX 0x004c6700
        "cuskeyd"
    },
    {
        // STRING: HYPERX 0x004c66f8
        "opjoy",
        // STRING: HYPERX 0x004c66f0
        "opjoyr",
        // STRING: HYPERX 0x004c66e8
        "opjoyd"
    }
};
// clang-format on

ConfigPoint g_aConfigButtonPos[2] = { { 44, 55 }, { 44, 87 } };

// The button loop's end bound is this table's address.
ConfigOptionLabel g_aConfigOptionLabels[7] = {
    // STRING: HYPERX 0x004c66e0
    { "opattkr", 44, 119 },
    // STRING: HYPERX 0x004c66d8
    { "opteamr", 44, 151 },
    // STRING: HYPERX 0x004c66d0
    { "perlwht", 44, 183 },
    // STRING: HYPERX 0x004c66c8
    { "musvol", 44, 215 },
    // STRING: HYPERX 0x004c66c0
    { "sfxvol", 44, 247 },
    // STRING: HYPERX 0x004c66b8
    { "brutwht", 44, 279 },
    // STRING: HYPERX 0x004c66b0
    { "diffwht", 44, 311 }
};

ConfigPoint g_aConfigDisplayPos[7] = {
    { 344, 124 }, { 344, 156 }, { 344, 188 }, { 344, 220 }, { 344, 252 }, { 344, 284 }, { 344, 316 }
};

ConfigPoint g_aConfigArrowPos[14] = {
    { 323, 124 }, { 536, 124 }, { 323, 156 }, { 536, 156 }, { 323, 188 }, { 536, 188 }, { 323, 220 }, { 536, 220 }, { 323, 252 }, { 536, 252 }, { 323, 284 }, { 536, 284 }, { 323, 316 }, { 536, 316 }
};

// STRING: HYPERX 0x004c66a8
const char* g_pszOptionArrowLeft1 = "oparlt1";

// STRING: HYPERX 0x004c66a0
const char* g_pszOptionArrowLeft2 = "oparlt2";

// STRING: HYPERX 0x004c6698
const char* g_pszOptionArrowLeft3 = "oparlt3";

// STRING: HYPERX 0x004c6690
const char* g_pszOptionArrowRight1 = "oparrt1";

// STRING: HYPERX 0x004c6688
const char* g_pszOptionArrowRight2 = "oparrt2";

// STRING: HYPERX 0x004c6680
const char* g_pszOptionArrowRight3 = "oparrt3";

const char* g_apAttackModeBitmapNames[2] = {
    // STRING: HYPERX 0x004c6678
    "attack1",
    // STRING: HYPERX 0x004c6670
    "attack2",
};

const char* g_apTeammateModeBitmapNames[2] = {
    // STRING: HYPERX 0x004c6668
    "teamb1",
    // STRING: HYPERX 0x004c6660
    "teamb2",
};
