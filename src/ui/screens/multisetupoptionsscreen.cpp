#include "common/compat.h"
#include "input/inputevent.h"
#include "globals.h"
#include "ui/screen.h"
#include "ui/pausemenu.h"
#include "ui/textinputwidget.h"
#include "ui/widget.h"
#include "ui/screens/multisetupoptionsscreen.h"
#include "ui/loopflipbookwidget.h"
#include "ui/flipbookwidget.h"
#include "graphics/gdi.h"
#include "engine/game.h"

// FUNCTION: HYPERX 0x004793f0
MultiSetupOptionsScreen::MultiSetupOptionsScreen()
{
    GDI::BitmapSize bm;
    int             nFlag;
    m_hBackground = GDI::LoadBitmapAsBackground(g_pszMultiSetupBackground, &m_bPaletted);
    m_nLastWidget = MULTI_SETUP_TEXT_INPUT;

    // STRING: HYPERX 0x004c2e58
    m_hBitmap = CacheLookup("3gray3", &nFlag);
    GDI::GetBitmapDimensions(m_hBitmap, &bm);
    Widget* pWidget = AppendWidget(new LoopFlipbookWidget(540, 390, bm.nWidth, bm.nHeight, 0, 1, 6, 0, 6, 6, this, 0, -1));
    pWidget->AddFrameWithPos(m_hBackground, nFlag, bm.nWidth, bm.nHeight, 0, 0, pWidget->GetPositionX(), pWidget->GetPositionY());
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

    pWidget = AppendWidget(new FlipbookWidget(540, 390, 100, 29, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->InitAfterLoad();

    pWidget = AppendWidget(new FlipbookWidget(540, 419, 100, 29, 0, 0, 2, 0, 1, 2, this, -1, -1, -1));
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->AppendFrame(nullptr);
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c6490
    m_hBitmap = GDI::LoadBitmapHandle("opbar0");
    GDI::GetBitmapDimensions(m_hBitmap, &bm);
    pWidget = AppendWidget(new TextInputWidget(0, 0, bm.nWidth, bm.nHeight, 0, 0, 0, 0, 0, 0, this, -1));
    pWidget->AppendFrame(m_hBitmap);

    m_apWidgets[MULTI_SETUP_OKAY]->ClearFrames();
    // STRING: HYPERX 0x004c2168
    m_apWidgets[MULTI_SETUP_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okay"));
    // STRING: HYPERX 0x004c2160
    m_apWidgets[MULTI_SETUP_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okayr"));
    // STRING: HYPERX 0x004c2158
    m_apWidgets[MULTI_SETUP_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okayd"));

    m_apWidgets[MULTI_SETUP_CANCEL]->ClearFrames();
    // STRING: HYPERX 0x004c2150
    m_apWidgets[MULTI_SETUP_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Cancl"));
    // STRING: HYPERX 0x004c2148
    m_apWidgets[MULTI_SETUP_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Canclr"));
    // STRING: HYPERX 0x004c2140
    m_apWidgets[MULTI_SETUP_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Cancld"));
}

// FUNCTION: HYPERX 0x00479830
MultiSetupOptionsScreen::~MultiSetupOptionsScreen() = default;

// FUNCTION: HYPERX 0x00479850
void MultiSetupOptionsScreen::OnWidgetMessage(int nMsg, unsigned int nWidget)
{
    if (nMsg == INPUT_LEFT_UP)
    {
        switch (nWidget)
        {
            case MULTI_SETUP_OKAY:
                OnOkay();
                break;

            case MULTI_SETUP_CANCEL:
                OnCancel();
                break;
        }
    }
}

// STRING: HYPERX 0x004c6488
const char* g_pszMultiSetupBackground = "msbkgd";
