#include "common/compat.h"
#include "input/inputevent.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "ui/screen.h"
#include "graphics/gdi.h"
class AwardsUIScreen;
#include "ui/pausemenu.h"
#include "engine/gameengine.h"
#include "ui/uiglobals.h"
#include "ui/widget.h"
#include <cstdio>
#include "ui/screens/awardsuiscreen.h"
#include "ui/flipbookwidget.h"
#include "ui/loopflipbookwidget.h"
#include "ui/displaywidget.h"
#include "engine/game.h"
#include "sound/soundids.h"

constexpr auto AWARDS_STRIP = 36;
#define AWARDS_LAST_WIDGET AWARDS_STRIP
constexpr auto AWARDS_ICON_FIRST   = 23;
constexpr auto AWARDS_BUTTON_FIRST = 29;
constexpr auto AWARDS_TROPHY       = 35;
constexpr auto AWARDS_ROW_COUNT    = 6;
constexpr auto AWARDS_STRIP_FRAMES = 0x1f;

constexpr auto AWARDS_STRIP_X = 0xf6;
constexpr auto AWARDS_STRIP_Y = 0x29;

// FUNCTION: HYPERX 0x0041b870
AwardsUIScreen::AwardsUIScreen()
{
    Widget*         pWidget;
    GDI::BitmapSize bm;
    GDI::Bitmap*    hBitmap;
    int             i;

    m_nAward = -1;

    m_hBackground = GDI::LoadBitmapAsBackground(g_pszAwardsBackdrop, &m_bPaletted);
    m_nLastWidget = AWARDS_LAST_WIDGET;

    m_apWidgets[MATCHINFO_WIDGET_AWARDS]->SetDepressed(1);

    for (i = 0; i < AWARDS_ROW_COUNT; i++)
    {
        hBitmap = GDI::LoadBitmapHandle(g_aAwardIcons[i].pszName);
        GDI::GetBitmapDimensions(hBitmap, &bm);
        pWidget = AppendWidget(new DisplayWidget(g_aAwardIcons[i].nX, g_aAwardIcons[i].nY, 0, 0, this, -1));
        pWidget->AppendFrame(hBitmap);
        m_apWidgets[AWARDS_ICON_FIRST + i]->SetHidden(g_pGameEngine->HasAward(i) == 0);
    }

    for (i = 0; i < AWARDS_ROW_COUNT; i++)
    {
        hBitmap = GDI::LoadBitmapHandle(g_aAwardButtons[i].pszNormal);
        GDI::GetBitmapDimensions(hBitmap, &bm);
        pWidget = AppendWidget(new FlipbookWidget(g_aAwardButtons[i].nX,
                                                  g_aAwardButtons[i].nY,
                                                  bm.nWidth,
                                                  bm.nHeight,
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
        pWidget->AppendFrame(GDI::LoadBitmapHandle(g_aAwardButtons[i].pszRollover));
        pWidget->AppendFrame(GDI::LoadBitmapHandle(g_aAwardButtons[i].pszDepressed));
        m_apWidgets[AWARDS_BUTTON_FIRST + i]->SetHidden(g_pGameEngine->HasAward(i) == 0);
    }

    hBitmap = GDI::LoadBitmapHandle(g_apszAwardTrophy[0]);
    GDI::GetBitmapDimensions(hBitmap, &bm);
    AppendWidget(new FlipbookWidget(0, 0, bm.nWidth, bm.nHeight, 0, 0, 0, 0, 0, 0, this, -1, -1, -1));

    m_apWidgets[AWARDS_TROPHY]->SetHidden(1);

    hBitmap = GDI::LoadBitmapHandle(g_apszAwardStripPrefix[0]);
    GDI::GetBitmapDimensions(hBitmap, &bm);
    pWidget = AppendWidget(new LoopFlipbookWidget(AWARDS_STRIP_X, AWARDS_STRIP_Y, bm.nWidth, bm.nHeight, 0, 1, 0x1e, 0, 0x1e, 0x1e, this, 1, -1));

    for (i = 0; i < AWARDS_STRIP_FRAMES; i++)
    {
        pWidget->AppendFrame(nullptr);
    }

    pWidget->SetHidden(1);
}

// FUNCTION: HYPERX 0x0041bbb0
void AwardsUIScreen::OnExitTabClick()
{
    if (g_bChampFromJumbotron != 0)
    {
        g_bChampFromJumbotron = 0;

        Game::SetEngineState(ESTATE_DROME_EXTERIOR);
        return;
    }

    UnloadBitmaps();

    Game::SetEngineState(m_nNextState);
}

// Every arm ends by re-entering the screen, including the one that matched no button.

constexpr auto AWARDS_NO_SOUND = (-1);

// FUNCTION: HYPERX 0x0041bbf0
void AwardsUIScreen::OnOtherButton(int nMsg, unsigned int nId)
{
    if (nMsg != INPUT_LEFT_UP)
    {
        return;
    }

    switch (nId)
    {
        case AWARDS_BUTTON_FIRST:
            m_nAward                                             = 0;
            m_nAwardsView                                        = 0;
            m_apWidgets[AWARDS_STRIP]->m_flipbook.m_nSoundAtMark = AWARDS_NO_SOUND;
            break;

        case AWARDS_BUTTON_FIRST + 1:
            m_nAward                                             = 1;
            m_nAwardsView                                        = 0;
            m_apWidgets[AWARDS_STRIP]->m_flipbook.m_nMarkFrame   = 0xe;
            m_apWidgets[AWARDS_STRIP]->m_flipbook.m_nSoundAtMark = SOUND_SXVGRMA3;
            break;

        case AWARDS_BUTTON_FIRST + 2:
            m_nAward                                             = 2;
            m_nAwardsView                                        = 0;
            m_apWidgets[AWARDS_STRIP]->m_flipbook.m_nMarkFrame   = 8;
            m_apWidgets[AWARDS_STRIP]->m_flipbook.m_nSoundAtMark = SOUND_SXHHITE2;
            break;

        case AWARDS_BUTTON_FIRST + 3:
            m_nAward                                             = 3;
            m_nAwardsView                                        = 0;
            m_apWidgets[AWARDS_STRIP]->m_flipbook.m_nSoundAtMark = AWARDS_NO_SOUND;
            break;

        case AWARDS_BUTTON_FIRST + 4:
            m_nAward                                             = 4;
            m_nAwardsView                                        = 0;
            m_apWidgets[AWARDS_STRIP]->m_flipbook.m_nSoundAtMark = AWARDS_NO_SOUND;
            break;

        case AWARDS_BUTTON_FIRST + 5:
            m_nAward                                             = 5;
            m_nAwardsView                                        = 0;
            m_apWidgets[AWARDS_STRIP]->m_flipbook.m_nSoundAtMark = AWARDS_NO_SOUND;
            break;

        case AWARDS_TROPHY:
            m_nAwardsView = 1;
            break;
    }

    OnEnter();
}

// FUNCTION: HYPERX 0x0041bd30
void AwardsUIScreen::OnEnter()
{
    char szName[0x50];
    int  nSlot;

    if (m_nAwardsView != 0)
    {
        m_apWidgets[AWARDS_TROPHY]->SetHidden(1);
        m_apWidgets[AWARDS_TROPHY]->SetDirtyFlag(0);
        m_apWidgets[AWARDS_STRIP]->SetHidden(1);
        m_apWidgets[AWARDS_STRIP]->SetDirtyFlag(0);

        for (nSlot = 0; nSlot < AWARDS_ROW_COUNT; nSlot++)
        {
            m_apWidgets[AWARDS_ICON_FIRST + nSlot]->SetHidden(
                g_pGameEngine->HasAward(nSlot) == 0);
            m_apWidgets[AWARDS_ICON_FIRST + nSlot]->SetDirtyFlag(
                static_cast<unsigned char>(g_pGameEngine->HasAward(nSlot)));
            m_apWidgets[AWARDS_BUTTON_FIRST + nSlot]->SetHidden(
                g_pGameEngine->HasAward(nSlot) == 0);
            m_apWidgets[AWARDS_BUTTON_FIRST + nSlot]->SetDirtyFlag(
                static_cast<unsigned char>(g_pGameEngine->HasAward(nSlot)));
        }

        m_apWidgets[UI_WIDGET_PAGE]->ClearFrames();

        m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle(g_pszAwardsBackground));

        m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);
        m_apWidgets[UI_WIDGET_PAGE]->SetHidden(0);

        InitFrame();
        return;
    }

    for (nSlot = 0; nSlot < AWARDS_ROW_COUNT; nSlot++)
    {
        m_apWidgets[AWARDS_BUTTON_FIRST + nSlot]->SetHidden(1);
        m_apWidgets[AWARDS_ICON_FIRST + nSlot]->SetHidden(1);
    }

    m_apWidgets[AWARDS_STRIP]->ClearFrames();

    for (nSlot = 0; nSlot < AWARDS_STRIP_FRAMES; nSlot++)
    {
        // STRING: HYPERX 0x004bc1a4
        sprintf(szName, "%s%02d", g_apszAwardStripPrefix[m_nAward], nSlot);

        m_apWidgets[AWARDS_STRIP]->AppendFrame(GDI::LoadBitmapHandle(szName));
    }

    m_apWidgets[AWARDS_STRIP]->SetDirtyFlag(1);
    m_apWidgets[AWARDS_STRIP]->SetHidden(0);
    m_apWidgets[AWARDS_STRIP]->SetDepressed(1);

    m_apWidgets[AWARDS_TROPHY]->SetHidden(0);
    m_apWidgets[AWARDS_TROPHY]->ClearFrames();

    m_apWidgets[AWARDS_TROPHY]->AppendFrame(GDI::LoadBitmapHandle(g_apszAwardTrophy[m_nAward]));

    m_apWidgets[AWARDS_TROPHY]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);

    InitFrame();
}

// The second of the image's two pointers to "awbkgd".
const char* g_pszAwardsBackdrop   = "awbkgd";
const char* g_pszAwardsBackground = "awbkgd";

AwardIcon g_aAwardIcons[6] = {
    // STRING: HYPERX 0x004bc194
    { "icortw", 285, 64 },
    // STRING: HYPERX 0x004bc18c
    { "icoanvl", 208, 152 },
    // STRING: HYPERX 0x004bc184
    { "icocrus", 360, 152 },
    // STRING: HYPERX 0x004bc17c
    { "icoarch", 133, 242 },
    // STRING: HYPERX 0x004bc174
    { "icosupr", 284, 242 },
    // STRING: HYPERX 0x004bc16c
    { "icorazr", 434, 242 }
};

AwardButton g_aAwardButtons[6] = {
    // STRING: HYPERX 0x004bc168
    { "rtw",
      // STRING: HYPERX 0x004bc160
      "rtwr",
      // STRING: HYPERX 0x004bc158
      "rtwd",
      284,
      129 },
    // STRING: HYPERX 0x004bc150
    { "anvil",
      // STRING: HYPERX 0x004bc148
      "anvilr",
      // STRING: HYPERX 0x004bc140
      "anvild",
      207,
      217 },
    // STRING: HYPERX 0x004bc138
    { "crush",
      // STRING: HYPERX 0x004bc130
      "crushr",
      // STRING: HYPERX 0x004bc128
      "crushd",
      360,
      217 },
    // STRING: HYPERX 0x004bc120
    { "arch",
      // STRING: HYPERX 0x004bc118
      "archr",
      // STRING: HYPERX 0x004bc110
      "archd",
      138,
      308 },
    // STRING: HYPERX 0x004bc108
    { "super",
      // STRING: HYPERX 0x004bc100
      "superr",
      // STRING: HYPERX 0x004bc0f8
      "superd",
      283,
      308 },
    // STRING: HYPERX 0x004bc0f0
    { "razor",
      // STRING: HYPERX 0x004bc0e8
      "razorr",
      // STRING: HYPERX 0x004bc0e0
      "razord",
      434,
      308 }
};

// The awards constructor bounds a loop on this table's address.
const char* g_apszAwardTrophy[6] = { "awtrtw", "awtanvl", "awtcrus", "awtarch", "awtsupr", "awtrazr" };

const char* g_apszAwardStripPrefix[6] = { "rtw", "anvil", "crush", "archr", "super", "razor" };
