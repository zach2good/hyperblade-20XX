#include "common/compat.h"
#include "infobar.h"
#include "ui/infobarbase.h"
#include "common/stub.h"

// FUNCTION: HYPERX 0x00452130
InfoBarBase* __cdecl InfoBarBase::Create(HDC hDC)
{
    return new InfoBar(hDC);
}
