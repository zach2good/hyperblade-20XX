#ifndef HYPERBLADE_DISPLAY_H
#define HYPERBLADE_DISPLAY_H
#include "common/compat.h"

namespace Display
{

int  Initialize(HWND hWnd);
int  ChangeColorDepth(int nDepth);
void ApplyRenderOptions(int bTexEnabled, int bBilinearFilter);

} // namespace Display

#endif
