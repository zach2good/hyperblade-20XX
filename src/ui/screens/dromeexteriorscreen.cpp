// An empty team background falls back to a fixed name; a negative round flag selects entry zero.

#include "common/compat.h"
#include "input/inputevent.h"
#include "engine/teamblock.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/screen.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "ui/torchbitmaps.h"
#include "engine/gameengine.h"
#include "engine/teamsblock.h"
#include "ui/displaywidget.h"
#include "ui/flipbookwidget.h"
#include "ui/saveobject.h"
#include "graphics/gdi.h"
#include "engine/game.h"

// FUNCTION: HYPERX 0x0048ef70
DromeExteriorScreen::~DromeExteriorScreen() = default;

// FUNCTION: HYPERX 0x0048efa0
DromeExteriorScreen::DromeExteriorScreen()
{
    const char* pszName;
    int         nRound;

    TeamsBlock* pList = g_pGameEngine->m_pTeamTable;

    if (static_cast<short>(g_nDromeArena) < pList->GetTeamCount())
    {
        pszName = pList->m_ppTeams[static_cast<short>(g_nDromeArena)]->szDromeExt;
    }
    else
    {
        pszName = nullptr;
    }

    if (*pszName == 0)
    {
        // STRING: HYPERX 0x004c8958
        pszName = "Dromext";
    }

    m_hBackground = GDI::LoadBitmapAsBackground(pszName, &m_bPaletted);
    m_nLastWidget = 2;

    if (g_nDromeTeamAway == g_nDromeArena)
    {
        pList = g_pGameEngine->m_pTeamTable;

        if (static_cast<short>(g_nDromeTeamAway) < pList->GetTeamCount())
        {
            pszName = pList->m_ppTeams[static_cast<short>(g_nDromeTeamAway)]->szTsLogo;
        }
        else
        {
            pszName = nullptr;
        }
    }
    else
    {
        pList = g_pGameEngine->m_pTeamTable;

        if (static_cast<short>(g_nDromeTeamHome) < pList->GetTeamCount())
        {
            pszName = pList->m_ppTeams[static_cast<short>(g_nDromeTeamHome)]->szTsLogo;
        }
        else
        {
            pszName = nullptr;
        }
    }

    m_hBitmap = GDI::LoadBitmapHandle(pszName);
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    Widget* pWidget = AppendWidget(new DisplayWidget(0x2d, 0x4f, 0, 0, this, -1));
    pWidget->AppendFrame(m_hBitmap);

    if (g_nDromeTeamAway == g_nDromeArena)
    {
        nRound = g_nDromeTeamHome;
    }
    else
    {
        nRound = g_nDromeTeamAway;
    }

    m_hBitmap = GDI::LoadBitmapHandle(g_apszVersusBanner[nRound]);
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new DisplayWidget(0x2f, 0x11c, 0, 0, this, -1));
    pWidget->AppendFrame(m_hBitmap);

    m_hBitmap = GDI::LoadBitmapHandle(g_pszKioskWhite);
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new FlipbookWidget(0, 0x161, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(m_hBitmap);
    pWidget->AppendFrame(GDI::LoadBitmapHandle(g_pszKioskBlue));
    pWidget->AppendFrame(GDI::LoadBitmapHandle(g_pszKioskGreen));
    pWidget->InitAfterLoad();

    if (g_nPlayerMode == kPlayerModeTournament)
    {
        m_nLastWidget = 3;

        nRound = g_pGameEngine->m_pSaveObject->GetBracketPos();

        if (static_cast<unsigned int>(nRound) >= 0xd)
        {
            nRound = 0;
        }

        m_hBitmap = GDI::LoadBitmapHandle(g_apszMatchBanner[nRound]);
        GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
        pWidget = AppendWidget(new DisplayWidget(0x2c, 0xd, 0, 0, this, -1));
        pWidget->AppendFrame(m_hBitmap);
    }
}

// FUNCTION: HYPERX 0x0048f2c0
void DromeExteriorScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    if (nId != 2)
    {
        if (nId > kWidgetIDMouse - 1 && nId <= kWidgetIDReturn)
        {
            if (nMsg == kInputKeyUp || nMsg == kInputKeyDown || (m_bPointerOnWidget == 0 && nMsg == kInputLeftUp))
            {
                Game::SetEngineState(kEstateMatchStart);

                InitFrame();
            }
        }

        return;
    }

    if (nMsg == kInputLeftUp)
    {
        ShowMatchInfo();
    }
}

int g_nSetupReturnState = kEstateLobby;

// STRING: HYPERX 0x004c871c
const char* g_pszKioskWhite = "kioswht";

// STRING: HYPERX 0x004c8714
const char* g_pszKioskBlue = "kiosblu";

// STRING: HYPERX 0x004c870c
const char* g_pszKioskGreen = "kiosgrn";

const char* g_apszVersusBanner[14] = {
    "vschi", "vshou", "vsjam", "vsveg", "vslnd", "vslos", "vsmos", "vsmun", "vsnem", "vsnew", "vssea", "vstok", "vstor", "vswar"
};

// Thirteen: the fourteenth word in the image is the string pool after the table.
const char* g_apszMatchBanner[13] = {
    "match01", "match02", "match03", "match04", "match05", "match06", "match07", "match08", "match09", "match10", "match11", "matchFN", "matchSP"
};
