#ifndef HYPERBLADE_MAINWINDOW_H
#define HYPERBLADE_MAINWINDOW_H

#include "common/compat.h"

namespace MainWindow
{

int     OnCreate();
LRESULT OnCommand(HWND hWnd, unsigned int nCommand, LPARAM lParam, int nNotify);

} // namespace MainWindow

#endif
