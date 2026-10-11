// Releasing a pooled owner through its own first slot.

#include "compat.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x00414b00
PooledOwnerRoot* PooledOwnerRoot::Destroy()
{
    delete this;

    return 0;
}
