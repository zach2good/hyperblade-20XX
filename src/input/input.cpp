#include "common/compat.h"
#include "input/input.h"

// FUNCTION: HYPERX1000 0x004c0d10
// FUNCTION: HYPERX1001 0x0044a120
int Input::IsShiftKeyDown(void)
{
    return GetKeyState(VK_SHIFT) < 0;
}

// FUNCTION: HYPERX1000 0x004c0d30
// FUNCTION: HYPERX1001 0x0044a140
int Input::IsCtrlKeyDown(void)
{
    return GetKeyState(VK_CONTROL) < 0;
}
