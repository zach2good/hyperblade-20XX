// One on One, the original menu's unhandled mode: one dromer a team sits out in the plans' own Sideln.

#ifndef HYPERBLADE_ONEONONE_H
#define HYPERBLADE_ONEONONE_H

namespace OneOnOne
{

bool On();

// Takes effect at the next restart; Two on Two is off.
void Set(bool bOn);

// Each period start in an exhibition match; seasons keep their rosters, as their saved records expect.
void ParkSeconds();

bool Parked(int nSlot);

// Before a restart into Two on Two: the sidelined dromers come back healthy.
void Unpark();

} // namespace OneOnOne

#endif
