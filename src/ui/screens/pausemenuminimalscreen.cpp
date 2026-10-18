#include "common/compat.h"
#include "input/inputevent.h"
#include "ui/pausemenu.h"
#include "app/gameapp.h"
#include "ui/screens/pausemenuminimalscreen.h"
#include "graphics/gdi.h"

// FUNCTION: HYPERX 0x00443950
PauseMenuMinimalScreen::~PauseMenuMinimalScreen() = default;

// FUNCTION: HYPERX 0x00443980
PauseMenuMinimalScreen::PauseMenuMinimalScreen()
{
    m_nLastWidget = PAUSE_MINIMAL_QUIT;

    Init();
}

// FUNCTION: HYPERX 0x004439e0
void PauseMenuMinimalScreen::Init()
{
    GDI::BitmapSize bm;

    // STRING: HYPERX 0x004c1fc0
    m_hBackground = GDI::LoadBitmapAsBackground("escape2", &m_bPaletted);

    // STRING: HYPERX 0x004c1f68
    auto* hBitmap = GDI::LoadBitmapHandle("Esretur");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    Widget* pWidget = AppendWidget(new ButtonWidget(0, 163, 210, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1f5c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esreturr"));
    // STRING: HYPERX 0x004c1f50
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esreturd"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c1f48
    hBitmap = GDI::LoadBitmapHandle("Esquit");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    pWidget = AppendWidget(new ButtonWidget(0, 163, 267, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1f40
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esquitr"));
    // STRING: HYPERX 0x004c1f38
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esquitd"));
    pWidget->InitAfterLoad();
}

// FUNCTION: HYPERX 0x00443ba0
void PauseMenuMinimalScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case PAUSE_MINIMAL_RETURN:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionReturnTraining();
            }
            break;

        case PAUSE_MINIMAL_QUIT:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionQuit();
            }
            break;

        case kWidgetIDMouse:
        case kWidgetIDReturn:
            break;

        default:
            Screen::OnWidgetMessage(nMsg, nId);
            break;
    }
}
