#include "common/compat.h"
#include "infobar.h"
#include "ui/infobarbase.h"
#include "hud.h"
#include "common/stub.h"

// FUNCTION: HYPERX 0x00452130
InfoBarBase* __cdecl InfoBarBase::Create(int nKind, HDC hDC)
{
    if (nKind == 0)
    {
        return new HUD(0);
    }

    return new InfoBar(hDC);
}
