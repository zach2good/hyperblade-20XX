#include "common/compat.h"
#include "input/input.h"

// FUNCTION: HYPERX 0x0044a120
int Input::IsShiftKeyDown(void)
{
    return GetKeyState(VK_SHIFT) < 0;
}

// FUNCTION: HYPERX 0x0044a140
int Input::IsCtrlKeyDown(void)
{
    return GetKeyState(VK_CONTROL) < 0;
}
