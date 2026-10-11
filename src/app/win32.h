#ifndef HYPERBLADE_WIN32_H
#define HYPERBLADE_WIN32_H

#include "common/compat.h"

namespace Win32
{

int  AskYesNo(const char* pszText, const char* pszCaption);
int  IsCDRomDrive(const char* pszRoot);
int  GetWindowTextAlloc(HWND hWnd, char** ppszOut);
void CenterWindow(HWND hWnd, HWND hParent);

} // namespace Win32

#endif
