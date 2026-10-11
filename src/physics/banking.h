// The banking globals PhysicsBody::ApplyBankForce works in.

#ifndef HYPERBLADE_PHYSICS_BANKING_H
#define HYPERBLADE_PHYSICS_BANKING_H

#include "engine/devicector.h"

struct Vec3;

// The banking scratchpad.
// GLOBAL: HYPERX 0x004cb388
extern Vec3 g_vBankRight;

// Which of the two directions counts as banking right.
// GLOBAL: HYPERX 0x004cb3b0
extern short g_nBankDirRight;

#endif
