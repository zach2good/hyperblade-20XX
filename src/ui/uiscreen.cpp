#include "common/compat.h"
#include "input/inputevent.h"
#include "framepos.h"
#include "widget.h"
#include "ui/screen.h"
#include "globals.h"
#include "ui/pausemenu.h"
#include "engine/teamsblock.h"
#include "common/pathstring.h"
#include "engine/gameengine.h"
#include <filesystem>
#include <system_error>
#include "uiscreen.h"
#include "sound/audiomgr.h"
#include "app/gameapp.h"
#include "loopflipbookwidget.h"
#include "flipbookwidget.h"
#include "graphics/gdi.h"
#include "common/stub.h"
#include "sound/soundids.h"

// The flag array goes out through operator delete.
// FUNCTION: HYPERX 0x0044d4b0
UIScreen::~UIScreen()
{
    if (m_pLockedFlags != 0)
    {
        delete[] m_pLockedFlags;

        m_pLockedFlags = 0;
    }
}

// FUNCTION: HYPERX 0x0044d540
UIScreen::UIScreen()
{
    g_pAudioMgr->PlaySpecific(SOUND_SCREENON);

    m_nLastWidget = UI_WIDGET_PAGE;

    if (g_nSetupBitmapsUnloaded != 0)
    {
        PreloadBitmaps();
    }

    m_pLockedFlags = 0;
    m_nLockedCount = 0;

    BuildLockedFlagsArray();

    InitWidgets();
}

// Widgets that show nothing yet get three empty frames so their indices line up.
// FUNCTION: HYPERX 0x0044d5d0
void UIScreen::InitWidgets()
{
    GDI::BitmapSize bm;
    int             nFlag;
    // STRING: HYPERX 0x004c2eb8
    m_hBackground = GDI::LoadBitmapAsBackground("Hviewl", &m_bPaletted);

    // STRING: HYPERX 0x004c2eb0
    m_hBitmap = CacheLookup("1data3", &nFlag);
    GDI::GetBitmapDimensions(m_hBitmap, &bm);
    Widget* pWidget = AppendWidget(new LoopFlipbookWidget(0, 362, bm.bmWidth, bm.bmHeight, 0, 1, 6, 0, 6, 6, this, 0, -1));
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2ea8
    m_hBitmap = CacheLookup("1data4", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2ea0
    m_hBitmap = CacheLookup("1data5", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e98
    m_hBitmap = CacheLookup("1gray6", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e90
    m_hBitmap = CacheLookup("1data1", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e88
    m_hBitmap = CacheLookup("1data2", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    m_hBitmap = CacheLookup("1data3", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c2e80
    m_hBitmap = CacheLookup("2five3", &nFlag);
    GDI::GetBitmapDimensions(m_hBitmap, &bm);
    pWidget = AppendWidget(new LoopFlipbookWidget(104, 428, bm.bmWidth, bm.bmHeight, 0, 1, 6, 0, 6, 6, this, 0, -1));
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e78
    m_hBitmap = CacheLookup("2five4", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e70
    m_hBitmap = CacheLookup("2five5", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c21b0
    m_hBitmap = CacheLookup("2gray6", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e68
    m_hBitmap = CacheLookup("2five1", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e60
    m_hBitmap = CacheLookup("2five2", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    m_hBitmap = CacheLookup("2five3", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c2e58
    m_hBitmap = CacheLookup("3gray3", &nFlag);
    GDI::GetBitmapDimensions(m_hBitmap, &bm);
    pWidget = AppendWidget(new LoopFlipbookWidget(540, 390, bm.bmWidth, bm.bmHeight, 0, 1, 6, 0, 6, 6, this, 0, -1));
    pWidget->AddFrameWithPos(m_hBackground, nFlag, bm.bmWidth, bm.bmHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e50
    m_hBitmap = CacheLookup("3gray4", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e48
    m_hBitmap = CacheLookup("3gray5", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e40
    m_hBitmap = CacheLookup("3gray6", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e38
    m_hBitmap = CacheLookup("3game1", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e30
    m_hBitmap = CacheLookup("3game2", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c2e28
    m_hBitmap = CacheLookup("4gray3", &nFlag);
    GDI::GetBitmapDimensions(m_hBitmap, &bm);
    pWidget = AppendWidget(new LoopFlipbookWidget(106, 389, bm.bmWidth, bm.bmHeight, 0, 1, 6, 0, 6, 6, this, 0, -1));
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e20
    m_hBitmap = CacheLookup("4gray4", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e18
    m_hBitmap = CacheLookup("4gray5", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e10
    m_hBitmap = CacheLookup("4gray6", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e08
    m_hBitmap = CacheLookup("4stat1", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2e00
    m_hBitmap = CacheLookup("4stat2", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    // STRING: HYPERX 0x004c2df8
    m_hBitmap = CacheLookup("4stat3", &nFlag);
    pWidget->AddFrameBitmap(m_hBitmap, nFlag);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(0, 2, 362, 100, 29, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(0, 2, 391, 100, 29, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(0, 2, 420, 100, 29, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(0, 2, 449, 100, 29, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(1, 114, 428, 80, 49, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(1, 197, 428, 80, 49, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(1, 280, 428, 80, 49, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(1, 363, 428, 80, 49, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(1, 446, 428, 80, 49, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(1, 529, 428, 80, 49, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget->SetHidden(1);
    pWidget = AppendWidget(new FlipbookWidget(540, 390, 100, 29, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new FlipbookWidget(540, 419, 100, 29, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new FlipbookWidget(218, 391, 27, 31, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new FlipbookWidget(395, 391, 27, 31, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(2, 115, 391, 101, 32, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(2, 218, 391, 101, 32, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(2, 321, 391, 101, 32, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new ButtonWidget(2, 424, 391, 101, 32, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new LoopFlipbookWidget(0, 0, 640, 353, 0, 0, 0, 0, 0, 0, this, 0, -1));
    pWidget->AppendFrame(0);
    pWidget->InitAfterLoad();
}

// An unreadable folder counts as missing, as _access answered.
static bool IsMissing(const char* pszPath)
{
    std::error_code error;

    return !std::filesystem::exists(pszPath, error);
}

// A team is locked when its panel bitmap is missing. Probes run by team code, not id.
// FUNCTION: HYPERX 0x0044e5d0
void UIScreen::BuildLockedFlagsArray()
{
    int nTeam;

    if (m_pLockedFlags == 0)
    {
        m_pLockedFlags = new int[g_pGameEngine->m_pTeamTable->GetTeamCount()];
    }

    for (nTeam = 0; nTeam < g_pGameEngine->m_pTeamTable->GetTeamCount(); nTeam++)
    {
        m_pLockedFlags[nTeam] = 0;
    }

    m_nLockedCount = 0;

    PathString strPath(MAX_PATH_LENGTH);

    // STRING: HYPERX 0x004c2f28
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Chiplcd", "bmp");
    m_pLockedFlags[0] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2f20
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Houplcd", "bmp");
    m_pLockedFlags[1] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2f18
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Jamplcd", "bmp");
    m_pLockedFlags[2] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2f10
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Lndplcd", "bmp");
    m_pLockedFlags[4] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2f08
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Losplcd", "bmp");
    m_pLockedFlags[5] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2f00
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Mosplcd", "bmp");
    m_pLockedFlags[6] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2ef8
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Munplcd", "bmp");
    m_pLockedFlags[7] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2ef0
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Nemplcd", "bmp");
    m_pLockedFlags[8] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2ee8
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Newplcd", "bmp");
    m_pLockedFlags[9] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2ee0
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Seaplcd", "bmp");
    m_pLockedFlags[10] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2ed8
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Tokplcd", "bmp");
    m_pLockedFlags[11] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2ed0
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Torplcd", "bmp");
    m_pLockedFlags[12] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2ec8
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Vegplcd", "bmp");
    m_pLockedFlags[3] = IsMissing(strPath.m_str.m_pchData);

    // STRING: HYPERX 0x004c2ec0
    strPath.m_str.FormatResourcePath(g_gameApp.m_strAssetsPath.m_str.m_pchData, "Warplcd", "bmp");
    m_pLockedFlags[13] = IsMissing(strPath.m_str.m_pchData);

    for (nTeam = 0; nTeam < g_pGameEngine->m_pTeamTable->GetTeamCount(); nTeam++)
    {
        m_nLockedCount += m_pLockedFlags[nTeam];
    }
}

// FUNCTION: HYPERX 0x0044ea10
void UIScreen::RefreshTabs()
{
    m_apWidgets[UI_WIDGET_TAB_FIRST]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_TAB_FIRST + 1]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_TAB_FIRST + 2]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_TAB_FIRST + 3]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);
}

// FUNCTION: HYPERX 0x0044ea50
void UIScreen::RefreshPageTabs()
{
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 1]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 2]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 3]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 4]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 5]->SetDirtyFlag(1);
}

// FUNCTION: HYPERX 0x0044eaa0
void UIScreen::RefreshConfirmButtons()
{
    m_apWidgets[UI_WIDGET_OKAY]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_CANCEL]->SetDirtyFlag(1);
}

// The stat row redraws before the arrows.
// FUNCTION: HYPERX 0x0044eac0
void UIScreen::RefreshStatRow()
{
    m_apWidgets[UI_WIDGET_STAT_FIRST]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_STAT_FIRST + 1]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_STAT_FIRST + 2]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_STAT_FIRST + 3]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetDirtyFlag(1);
}

// FUNCTION: HYPERX 0x0044eb10
void UIScreen::OnOtherButton(int, unsigned int)
{
    RELEASE_ASSERT(0);
}

// FUNCTION: HYPERX 0x0044eb20
void UIScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case UI_WIDGET_TAB_PLATE:
            if (nMsg == WIDGET_MSG_LOOP_DONE)
            {
                RefreshTabs();
            }
            break;

        case UI_WIDGET_PAGE_TAB_PLATE:
            if (nMsg == WIDGET_MSG_LOOP_DONE)
            {
                RefreshPageTabs();
            }
            break;

        case UI_WIDGET_CONFIRM_PLATE:
            if (nMsg == WIDGET_MSG_LOOP_DONE)
            {
                RefreshConfirmButtons();
            }
            break;

        case UI_WIDGET_STAT_PLATE:
            if (nMsg == WIDGET_MSG_LOOP_DONE)
            {
                RefreshStatRow();
            }
            break;

        case UI_WIDGET_TAB_FIRST:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT1);

                OnFirstTabClick();
            }
            break;

        case UI_WIDGET_TAB_FIRST + 1:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT1);

                OnSecondTabClick();
            }
            break;

        case UI_WIDGET_TAB_FIRST + 2:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT1);

                OnThirdTabClick();
            }
            break;

        case UI_WIDGET_TAB_FIRST + 3:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT1);

                OnExitTabClick();
            }
            break;

        case UI_WIDGET_PAGE_TAB_FIRST:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT2);

                OnFirstPageTabClick();
            }
            break;

        case UI_WIDGET_PAGE_TAB_FIRST + 1:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT2);

                OnSecondPageTabClick();
            }
            break;

        case UI_WIDGET_PAGE_TAB_FIRST + 2:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT2);

                OnThirdPageTabClick();
            }
            break;

        case UI_WIDGET_PAGE_TAB_FIRST + 3:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT2);

                OnFourthPageTabClick();
            }
            break;

        case UI_WIDGET_PAGE_TAB_FIRST + 4:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT2);

                OnFifthPageTabClick();
            }
            break;

        case UI_WIDGET_PAGE_TAB_FIRST + 5:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT2);

                OnSixthPageTabClick();
            }
            break;

        case UI_WIDGET_OKAY:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT1);

                OnOkayClick();
            }
            break;

        case UI_WIDGET_CANCEL:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT1);

                OnCancelClick();
            }
            break;

        case UI_WIDGET_LEFT_ARROW:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT3);

                OnLeftArrowClick();
            }
            break;

        case UI_WIDGET_RIGHT_ARROW:
            if (nMsg == INPUT_LEFT_UP)
            {
                g_pAudioMgr->PlaySpecific(SOUND_SXBUTT3);

                OnRightArrowClick();
            }
            break;

        case UI_WIDGET_STAT_FIRST:
            if (nMsg == INPUT_LEFT_UP)
            {
                OnFirstStatClick();
            }
            break;

        case UI_WIDGET_STAT_FIRST + 1:
            if (nMsg == INPUT_LEFT_UP)
            {
                OnSecondStatClick();
            }
            break;

        case UI_WIDGET_STAT_FIRST + 2:
            if (nMsg == INPUT_LEFT_UP)
            {
                OnThirdStatClick();
            }
            break;

        case UI_WIDGET_STAT_FIRST + 3:
            if (nMsg == INPUT_LEFT_UP)
            {
                OnFourthStatClick();
            }
            break;

        case UI_WIDGET_PAGE:
        case WIDGET_ID_MOUSE:
        case WIDGET_ID_RETURN:
            break;

        default:
            OnOtherButton(nMsg, nId);
            break;
    }
}

// "2gray6" appears in two blocks and "4gamcl3" twice in a row.
// FUNCTION: HYPERX 0x0044ef00
void UIScreen::PreloadBitmaps()
{
    AddBitmap("1data3");

    AddBitmap("1data4");

    AddBitmap("1data5");

    AddBitmap("1gray6");

    AddBitmap("1data1");

    AddBitmap("1data2");

    AddBitmap("2five3");

    AddBitmap("2five4");

    AddBitmap("2five5");

    AddBitmap("2gray6");

    AddBitmap("2five1");

    AddBitmap("2five2");

    AddBitmap("3gray3");

    AddBitmap("3gray4");

    AddBitmap("3gray5");

    AddBitmap("3gray6");

    AddBitmap("3game1");

    AddBitmap("3game2");

    AddBitmap("4gray3");

    AddBitmap("4gray4");

    AddBitmap("4gray5");

    AddBitmap("4gray6");

    AddBitmap("4stat1");

    AddBitmap("4stat2");

    AddBitmap("4stat3");

    // STRING: HYPERX 0x004c2f58
    AddBitmap("4gamgr3");

    // STRING: HYPERX 0x004c2f50
    AddBitmap("4gamgr4");

    // STRING: HYPERX 0x004c2f48
    AddBitmap("4gamgr5");

    // STRING: HYPERX 0x004c2f40
    AddBitmap("4gamcl1");

    // STRING: HYPERX 0x004c2f38
    AddBitmap("4gamcl2");

    // STRING: HYPERX 0x004c2f30
    AddBitmap("4gamcl3");

    AddBitmap("4gamcl3");

    AddBitmap("2three3");

    AddBitmap("2three4");

    AddBitmap("2three5");

    AddBitmap("2gray6");

    AddBitmap("2three1");

    AddBitmap("2three2");
    g_nSetupBitmapsUnloaded = 0;
}

// FUNCTION: HYPERX 0x00488bd0
void UIScreen::ResetFrameOnLoopDone(int nMsg)
{
    if (nMsg == WIDGET_MSG_LOOP_DONE)
    {
        m_apWidgets[UI_WIDGET_OWN_FIRST]->SetCurrentFrame(0);
    }
}

int g_nSetupBitmapsUnloaded = 1;
