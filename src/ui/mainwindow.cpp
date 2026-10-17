#include "common/compat.h"
#include "engine/messageids.h"
#include "globals.h"
#include "app/gameapp.h"
#include "engine/gameengine.h"
#include "sound/audiomgr.h"
#include "app/frametimer.h"
#include "graphics/colorpalette.h"
#include "uiglobals.h"
#include "engine/engineobject.h"
#include "input/gamepad.h"
#include "engine/objectclasses.h"
#include "anim/animset.h"
#include "planvm/planvm.h"
#include "ui/mainwindow.h"
#include "engine/object.h"
#include "app/win32.h"
#include "app/wndproc.h"
#include "app/menuids.h"

extern unsigned char g_bEnergyOn;

// FUNCTION: HYPERX 0x0044d0d0
int MainWindow::OnCreate()
{
    g_pColorPalette->ReloadPalette(g_wPaletteMode);

    Gamepad::Init();

    return 1;
}

short g_wUnusedBesidePalette = 1;
short g_wPaletteMode         = 3;
