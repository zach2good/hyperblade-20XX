#include "common/compat.h"
#include "input/inputevent.h"
#include "ui/widget.h"
#include "ui/screen.h"
#include "ui/pausemenu.h"
#include "globals.h"
#include "app/gameapp.h"
#include "ui/screens/pausemenufullscreen.h"
#include "graphics/gdi.h"

// FUNCTION: HYPERX 0x004433c0
PauseMenuFullScreen::~PauseMenuFullScreen() = default;

// FUNCTION: HYPERX 0x004433f0
PauseMenuFullScreen::PauseMenuFullScreen()
{
    m_nLastWidget = PAUSE_FULL_QUIT;

    Init();
}

// FUNCTION: HYPERX 0x00443450
void PauseMenuFullScreen::Init()
{
    GDI::BitmapSize bm;

    // STRING: HYPERX 0x004c1fb8
    m_hBackground = GDI::LoadBitmapAsBackground("escape1", &m_bPaletted);

    // STRING: HYPERX 0x004c1fb0
    auto* hBitmap = GDI::LoadBitmapHandle("Essave");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    Widget* pWidget = AppendWidget(std::make_unique<ButtonWidget>(0, 163, 35, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1fa8
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Essaver"));
    // STRING: HYPERX 0x004c1fa0
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Essaved"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c1f98
    hBitmap = GDI::LoadBitmapHandle("Esops");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    pWidget = AppendWidget(std::make_unique<ButtonWidget>(0, 163, 94, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1f90
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esopsr"));
    // STRING: HYPERX 0x004c1f88
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esopsd"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c1f80
    hBitmap = GDI::LoadBitmapHandle("Esexit");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    pWidget = AppendWidget(std::make_unique<ButtonWidget>(0, 163, 151, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1f78
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esexitr"));
    // STRING: HYPERX 0x004c1f70
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esexitd"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c1f68
    hBitmap = GDI::LoadBitmapHandle("Esretur");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    pWidget = AppendWidget(std::make_unique<ButtonWidget>(0, 163, 210, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1f5c
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esreturr"));
    // STRING: HYPERX 0x004c1f50
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esreturd"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c1f48
    hBitmap = GDI::LoadBitmapHandle("Esquit");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    pWidget = AppendWidget(std::make_unique<ButtonWidget>(0, 163, 267, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1f40
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esquitr"));
    // STRING: HYPERX 0x004c1f38
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esquitd"));
    pWidget->InitAfterLoad();
}

// The id is compared unsigned, so an id above the table falls to the default.
// FUNCTION: HYPERX 0x00443820
void PauseMenuFullScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case PAUSE_FULL_OPTIONS:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionOptions();
            }
            break;

        case PAUSE_FULL_SAVE:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionSave();
            }
            break;

        case PAUSE_FULL_EXIT:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionExitToMenu();
            }
            break;

        case PAUSE_FULL_RETURN:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionReturn();
            }
            break;

        case PAUSE_FULL_QUIT:
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
