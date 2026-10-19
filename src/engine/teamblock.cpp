#include "common/compat.h"
#include "teamblock.h"
#include "objectdef.h"

// Every field starts zeroed, as the parser cleared the whole block over the constructor's work.
// FUNCTION: HYPERX 0x00484730
TeamBlock::TeamBlock() = default;

// FUNCTION: HYPERX 0x004847a0
TeamBlock::~TeamBlock() = default;
