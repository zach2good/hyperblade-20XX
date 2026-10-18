#include "common/compat.h"
#include "input/inputevent.h"
#include "ui/pausemenu.h"
#include "app/gameapp.h"
#include "ui/screens/pausemenunosavescreen.h"
#include "graphics/gdi.h"

// FUNCTION: HYPERX 0x00443c80
PauseMenuNoSaveScreen::~PauseMenuNoSaveScreen() = default;

// FUNCTION: HYPERX 0x00443cb0
PauseMenuNoSaveScreen::PauseMenuNoSaveScreen()
{
    m_nLastWidget = PAUSE_NO_SAVE_QUIT;

    Init();
}

// FUNCTION: HYPERX 0x00443d10
void PauseMenuNoSaveScreen::Init()
{
    GDI::BitmapSize bm;

    // STRING: HYPERX 0x004c1fc8
    m_hBackground = GDI::LoadBitmapAsBackground("escape3", &m_bPaletted);

    // STRING: HYPERX 0x004c1f98
    GDI::Bitmap* hBitmap = GDI::LoadBitmapHandle("Esops");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    Widget* pWidget = AppendWidget(new ButtonWidget(0, 163, 94, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1f90
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esopsr"));
    // STRING: HYPERX 0x004c1f88
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esopsd"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c1f80
    hBitmap = GDI::LoadBitmapHandle("Esexit");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    pWidget = AppendWidget(new ButtonWidget(0, 163, 151, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    pWidget->AppendFrame(hBitmap);
    // STRING: HYPERX 0x004c1f78
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esexitr"));
    // STRING: HYPERX 0x004c1f70
    pWidget->AppendFrame(GDI::LoadBitmapHandle("Esexitd"));
    pWidget->InitAfterLoad();

    // STRING: HYPERX 0x004c1f68
    hBitmap = GDI::LoadBitmapHandle("Esretur");
    GDI::GetBitmapDimensions(hBitmap, &bm);
    pWidget = AppendWidget(new ButtonWidget(0, 163, 210, bm.nWidth, bm.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
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

// FUNCTION: HYPERX 0x00444030
void PauseMenuNoSaveScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case PAUSE_NO_SAVE_OPTIONS:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionOptions();
            }
            break;

        case PAUSE_NO_SAVE_EXIT:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionExitToMenu();
            }
            break;

        case PAUSE_NO_SAVE_RETURN:
            if (nMsg == kInputLeftUp)
            {
                PauseMenu::ActionReturnNoSave();
            }
            break;

        case PAUSE_NO_SAVE_QUIT:
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
