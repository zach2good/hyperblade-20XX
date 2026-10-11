#ifndef HYPERBLADE_ANIMATIONPEEKDIALOG_H
#define HYPERBLADE_ANIMATIONPEEKDIALOG_H

#include "common/compat.h"

namespace AnimationPeekDialog
{

unsigned int __cdecl OnInit(HWND hDlg, unsigned int nParam, long lParam);
unsigned int __cdecl OnCommand(HWND hDlg, int nCommand, HWND hControl, int nNotify);

} // namespace AnimationPeekDialog

#endif
