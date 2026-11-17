#ifdef HYPERX_V100
#include "common/compat.h"
#include "globals.h"
#include "app/gameapp.h"
#include "app/pagemngm.h"

// FUNCTION: HYPERX1000 0x0040f160
void PageMngm::Swap(void)
{
    // STRING: HYPERX1000 0x0050cbf8
    g_gameApp.Error("Executing stub function in PageMngm: pg_swap!");
}

// FUNCTION: HYPERX1000 0x0040f1a0
void PageMngm::ClearBuffer(void)
{
    // STRING: HYPERX1000 0x0050cc30
    g_gameApp.Error("Executing stub function in PageMngm: ClearBuffer!");
}

// FUNCTION: HYPERX1000 0x0040f1c0
void PageMngm::Setup(void)
{
    // STRING: HYPERX1000 0x0050cc6c
    g_gameApp.Error("Executing stub function in PageMngm: pg_setup!");
}
#endif
