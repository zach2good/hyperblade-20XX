#include "common/compat.h"
#include "input/inputevent.h"
#include "globals.h"
#include "ui/screen.h"
#include "ui/saveload.h"
#include "ui/screens/saveloaduiscreen.h"
#include "engine/gameengine.h"
#include "ui/torchbitmaps.h"
#include "ui/pausemenu.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "ui/widget.h"
#include "engine/teamblock.h"
#include <cstdio>
#include <cstring>
#include "ui/displaywidget.h"
#include "ui/saveobject.h"
#include "ui/flipbookwidget.h"
#include "engine/objectclasses.h"
#include "graphics/gdi.h"
#include "engine/game.h"
#include "common/stub.h"
#include "planvm/damagegate.h"

constexpr auto SLOT_LABEL_BYTES = 0x100;

constexpr auto DESCRIPTION_MAX = 0x100;

// Kiosk mode past the career limit gets a frameless flipbook, keeping the numbering.

// One slot row: a fixed column, a fixed width and the row's own height.
constexpr auto SLOT_ROW_X     = 0x69;
constexpr auto SLOT_ROW_WIDTH = 0x1ab;

// FUNCTION: HYPERX 0x0046dc50
SaveLoadUIScreen::SaveLoadUIScreen()
{
    int nFlag;
    int nMaxSaves;
    int nX;
    int nY;

    m_nSlot                = -1;
    m_nSelectedLabelWidget = -1;

    m_apWidgets[UI_WIDGET_TAB_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 4]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(1);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE]->SetHidden(1);

    m_apWidgets[UI_WIDGET_OKAY]->ClearFrames();
    GDI::Bitmap* pBitmap = CacheLookup("Okay", &nFlag);
    m_apWidgets[UI_WIDGET_OKAY]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Okayr", &nFlag);
    m_apWidgets[UI_WIDGET_OKAY]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Okayd", &nFlag);
    m_apWidgets[UI_WIDGET_OKAY]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[UI_WIDGET_CANCEL]->ClearFrames();
    pBitmap = CacheLookup("Cancl", &nFlag);
    m_apWidgets[UI_WIDGET_CANCEL]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Canclr", &nFlag);
    m_apWidgets[UI_WIDGET_CANCEL]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Cancld", &nFlag);
    m_apWidgets[UI_WIDGET_CANCEL]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[UI_WIDGET_LEFT_ARROW]->ClearFrames();
    pBitmap = CacheLookup("Stleft", &nFlag);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Stleftr", &nFlag);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Stleftd", &nFlag);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->ClearFrames();
    pBitmap = CacheLookup("Strite", &nFlag);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Striter", &nFlag);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Strited", &nFlag);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AddFrameBitmap(pBitmap, nFlag);

    m_nLastWidget = SAVELOAD_LAST_WIDGET;

    // STRING: HYPERX 0x004c4f98
    GDI::Bitmap* const hHeader = GDI::LoadBitmapHandle("savload");
    GDI::GetBitmapDimensions(hHeader, &m_bitmap);
    const int nHeaderWidth = m_bitmap.nWidth;
    Widget*   pWidget      = AppendWidget(new DisplayWidget(0, 0, 0, 0, this, -1));
    pWidget->AppendFrame(hHeader);

    // STRING: HYPERX 0x004c4f90
    GDI::Bitmap* hBitmap = GDI::LoadBitmapHandle("savwht1");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0x58, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savwht1"));
    // STRING: HYPERX 0x004c4f88
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn1"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f80
    hBitmap = GDI::LoadBitmapHandle("savwht2");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0x6f, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4f78
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn2"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f70
    hBitmap = GDI::LoadBitmapHandle("savwht3");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0x86, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4f68
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn3"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f60
    hBitmap = GDI::LoadBitmapHandle("savwht4");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0x9d, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4f58
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn4"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f50
    hBitmap = GDI::LoadBitmapHandle("savwht5");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0xb4, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4f48
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn5"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f40
    hBitmap = GDI::LoadBitmapHandle("savwht6");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0xcb, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4f38
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn6"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f30
    hBitmap = GDI::LoadBitmapHandle("savwht7");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0xe2, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4f28
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn7"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f20
    hBitmap = GDI::LoadBitmapHandle("savwht8");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0xf9, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4f18
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn8"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f10
    hBitmap = GDI::LoadBitmapHandle("savwht9");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0x110, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4f08
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn9"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c4f00
    hBitmap = GDI::LoadBitmapHandle("savwht0");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, SLOT_ROW_X, 0x127, SLOT_ROW_WIDTH, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    pWidget->AppendFrame(nullptr);
    // STRING: HYPERX 0x004c4ef8
    pWidget->AppendFrame(GDI::LoadBitmapHandle("savgrn0"));
    pWidget->InitAfterLoad();

    if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_EASY)
    {
        nMaxSaves = 10;
    }
    else if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_MEDIUM)
    {
        nMaxSaves = 11;
    }
    else if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_HARD)
    {
        nMaxSaves = 12;
    }
    else
    {
        nMaxSaves = 12;
    }

    if (g_pGameEngine->IsTournamentMode() == 0 || g_pGameEngine->m_pSaveObject->GetBracketPos() <= nMaxSaves)
    {
        // STRING: HYPERX 0x004c4ef0
        hBitmap = GDI::LoadBitmapHandle("save");
        GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
        pWidget = AppendWidget(new FlipbookWidget(0x32, 400, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
        pWidget->AppendFrame(hBitmap);
        // STRING: HYPERX 0x004c4ee8
        pWidget->AppendFrame(GDI::LoadBitmapHandle("saver"));
        // STRING: HYPERX 0x004c4ee0
        pWidget->AppendFrame(GDI::LoadBitmapHandle("saved"));
        pWidget->InitAfterLoad();
    }
    else
    {
        AppendWidget(new FlipbookWidget(0x32, 400, 0, 0, 0, 0, 0, 0, 0, 0, this, -1, -1, -1));
    }

    // STRING: HYPERX 0x004c4ed8
    hBitmap = GDI::LoadBitmapHandle("load");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    pWidget = AppendWidget(new FlipbookWidget(0x140, 400, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c4ed0
    pWidget->AppendFrame(GDI::LoadBitmapHandle("loadr"));
    // STRING: HYPERX 0x004c4ec8
    pWidget->AppendFrame(GDI::LoadBitmapHandle("loadd"));
    pWidget->InitAfterLoad();

    const int nPitch = g_aSlotRows[1].nY - g_aSlotRows[0].nY;

    for (int nRow = 0; nRow < g_nSlotRowCount; nRow++)
    {
        nX      = g_aSlotRows[nRow].nX;
        nY      = g_aSlotRows[nRow].nY;
        pWidget = AppendWidget(new DisplayWidget(nX, nY, 0, 0, this, -1));
        pWidget->AppendFrameWithPos(hHeader, nHeaderWidth - nX, nPitch, 0, 0, nX, nY);
        pWidget->InitAfterLoad();
        pWidget->SetDirtyFlag(1);
    }

    // From the main menu the screen only loads.
    if (g_nPrevEngineState == ESTATE_MAIN_MENU)
    {
        m_apWidgets[SAVELOAD_SAVE]->SetHidden(1);
        m_apWidgets[SAVELOAD_SAVE]->SetDirtyFlag(0);
    }

    RefreshSlotLabels();
}

// FUNCTION: HYPERX 0x0046e950
SaveLoadUIScreen::~SaveLoadUIScreen() = default;

// FUNCTION: HYPERX 0x0046e960
void SaveLoadUIScreen::OnOtherButton(int nMessage, unsigned int nWidget)
{
    switch (nWidget)
    {
        case SAVELOAD_SLOT_FIRST:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 0;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 1:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 1;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 1;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 2:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 2;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 2;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 3:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 3;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 3;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 4:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 4;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 4;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 5:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 5;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 5;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 6:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 6;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 6;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 7:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 7;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 7;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 8:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 8;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 8;

                return;
            }

            break;

        case SAVELOAD_SLOT_FIRST + 9:
            if (nMessage == INPUT_LEFT_UP)
            {
                m_nSlot                = 9;
                m_nSelectedLabelWidget = SAVELOAD_LABEL_FIRST + 9;

                return;
            }

            break;

        case SAVELOAD_SAVE:
            if (nMessage == INPUT_LEFT_UP)
            {
                SaveGame();

                return;
            }

            break;

        case SAVELOAD_LOAD:
            if (nMessage == INPUT_LEFT_UP)
            {
                LoadGame();

                return;
            }

            break;

        default:
            RELEASE_ASSERT(0);
    }
}

// "Match <n> <home> VS. <away>". Each copy bound is the larger of two lengths.

constexpr auto PART_MAX = 0x100;

// FUNCTION: HYPERX 0x0046eb50
void SaveLoadUIScreen::SaveGame()
{
    char szDescription[DESCRIPTION_MAX];

    if (g_nPlayerMode != PLAYER_MODE_TOURNAMENT || m_nSlot == SLOT_NONE)
    {
        return;
    }

    g_pGameEngine->SetTeams(static_cast<short>(g_nDromeTeamHome), static_cast<short>(g_nDromeTeamAway));

    g_pGameEngine->SetArena(static_cast<short>(g_nDromeArena));

    FormatSaveDescription(szDescription, DESCRIPTION_MAX);

    g_pGameEngine->WriteSaveFile(m_nSlot, szDescription);

    m_apWidgets[m_nSelectedLabelWidget]->SetDirtyFlag(1);

    RefreshSlotLabels();

    InitFrame();
}

// FUNCTION: HYPERX 0x0046ebf0
void SaveLoadUIScreen::LoadGame()
{
    if (m_nSlot == SLOT_NONE)
    {
        return;
    }

    g_pGameEngine->ReadSaveFile(m_nSlot);

    g_nPlayerMode = PLAYER_MODE_TOURNAMENT;
}

// FUNCTION: HYPERX 0x0046ec20
void SaveLoadUIScreen::RefreshFrames()
{
    m_apWidgets[UI_WIDGET_PAGE]->ClearFrames();

    m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);
}

// FUNCTION: HYPERX 0x0046ec40
void SaveLoadUIScreen::RefreshSlotLabels()
{
    char szLabel[SLOT_LABEL_BYTES];

    for (int nRow = 0; nRow < g_nSlotRowCount; nRow++)
    {
        Widget* const pWidget = m_apWidgets[SAVELOAD_LABEL_FIRST + nRow];

        g_pGameEngine->ReadSlotLabel(nRow, szLabel, SLOT_LABEL_BYTES);

        pWidget->SetText(szLabel);
        pWidget->SetDirtyFlag(1);
    }
}

// FUNCTION: HYPERX 0x0046eca0
void SaveLoadUIScreen::FormatSaveDescription(char* pszOut, int nMax)
{
    char szPart[PART_MAX];

    const int nRound = g_pGameEngine->m_pSaveObject->GetBracketPos();

    // STRING: HYPERX 0x004c4fa8
    strcpy(szPart, "Match ");

    int nOut = 0;
    int nAt  = 0;

    while (nOut < nMax || nAt < PART_MAX)
    {
        pszOut[nOut] = szPart[nAt];

        if (pszOut[nOut] == 0)
        {
            break;
        }

        nOut++;
        nAt++;
    }

    // STRING: HYPERX 0x004bb814
    sprintf(szPart, "%d", nRound + 1);

    nAt = 0;

    while (nOut < nMax || nAt < PART_MAX)
    {
        pszOut[nOut] = szPart[nAt];

        if (pszOut[nOut] == 0)
        {
            break;
        }

        nOut++;
        nAt++;
    }

    // STRING: HYPERX 0x004bc410
    strcpy(szPart, " ");

    nAt = 0;

    while (nOut < nMax || nAt < PART_MAX)
    {
        pszOut[nOut] = szPart[nAt];

        if (pszOut[nOut] == 0)
        {
            break;
        }

        nOut++;
        nAt++;
    }

    strcpy(szPart, g_pGameEngine->m_pTeamA->GetName());

    nAt = 0;

    while (nOut < nMax || nAt < PART_MAX)
    {
        pszOut[nOut] = szPart[nAt];

        if (pszOut[nOut] == 0)
        {
            break;
        }

        nOut++;
        nAt++;
    }

    // STRING: HYPERX 0x004c4fa0
    strcpy(szPart, " VS. ");

    nAt = 0;

    while (nOut < nMax || nAt < PART_MAX)
    {
        pszOut[nOut] = szPart[nAt];

        if (pszOut[nOut] == 0)
        {
            break;
        }

        nOut++;
        nAt++;
    }

    strcpy(szPart, g_pGameEngine->m_pTeamB->GetName());

    nAt = 0;

    while (nOut < nMax || nAt < PART_MAX)
    {
        pszOut[nOut] = szPart[nAt];

        if (pszOut[nOut] == 0)
        {
            break;
        }

        nOut++;
        nAt++;
    }
}

int g_nSlotRowCount = 10;

// Ten rows, one per save slot, evenly spaced down a fixed column.
SlotRow g_aSlotRows[10] = {
    { 0x91, 0x59 }, { 0x91, 0x70 }, { 0x91, 0x87 }, { 0x91, 0x9e }, { 0x91, 0xb5 }, { 0x91, 0xcc }, { 0x91, 0xe3 }, { 0x91, 0xfa }, { 0x91, 0x111 }, { 0x91, 0x128 }
};
