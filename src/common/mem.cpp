#include "compat.h"
#include "common/mem.h"
#include "common/staticinit.h"
#include <string.h>

float g_flUnusedDisplayModeA = g_flZeroDisplayMode;
float g_flUnusedDisplayModeB = g_flZeroDisplayMode;
float g_flUnusedDisplayModeC = g_flZeroDisplayMode;

// FUNCTION: HYPERX 0x00439590
void __cdecl Mem::ZeroBlock192(void* pBlock)
{
    memset(pBlock, 0, 192);
}

// FUNCTION: HYPERX 0x004395c0
void __cdecl Mem::ZeroBlockUnrolled128(void* pBlock)
{
    memset(pBlock, 0, 128);
}
