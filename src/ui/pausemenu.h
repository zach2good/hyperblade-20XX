#ifndef HYPERBLADE_PAUSEMENU_H
#define HYPERBLADE_PAUSEMENU_H

#include "common/compat.h"
#include "globals.h"
#include "ui/holdingwidget.h"

namespace PauseMenu
{

void ActionSave();
void ActionReturn();
void ActionReturnTraining();
void ActionOptions();
void ActionExitToMenu();
void ActionReturnNoSave();
void ActionQuit();

} // namespace PauseMenu

#include "ui/buttonwidget.h"
#include "widget.h"
#include "ui/screen.h"
#include "graphics/gdi.h"

// The sixty animation frames are the prefix formatted with a two-digit index.
// GLOBAL: HYPERX 0x004c1eb0
extern const char* g_pszFinaleBackground;
// GLOBAL: HYPERX 0x004c1eb4
extern const char* g_pszFinaleAnimPrefix;

// GLOBAL: HYPERX 0x004c6484
extern const char* g_pszMultiSetupBackground;

#endif
