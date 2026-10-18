#include "common/compat.h"
#include "infobar.h"
#include "ui/infobarbase.h"
#include "common/stub.h"

// FUNCTION: HYPERX 0x00452130
std::unique_ptr<InfoBarBase> InfoBarBase::Create()
{
    return std::make_unique<InfoBar>();
}
