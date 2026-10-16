#ifndef HYPERBLADE_DISPLAY_H
#define HYPERBLADE_DISPLAY_H
#include "common/compat.h"

namespace Display
{

int  Initialize(HWND hWnd, HDC hDisplayDC);
LONG ChangeColorDepth(int nDepth);
void ApplyRenderOptions(HMENU hMenu, int bTexEnabled, int bBilinearFilter);

} // namespace Display

#endif
