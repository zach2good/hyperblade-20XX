// One on One, the original menu's unhandled mode: each team skates one dromer, the goalies stay.
// The second dromer on each team sits out in the plans' own sidelined state, which every teammate
// and opponent test already skips as injured.

#ifndef HYPERBLADE_ONEONONE_H
#define HYPERBLADE_ONEONONE_H

namespace OneOnOne
{

bool On();

// Takes effect from the next period, or at once through a restart. Two on Two is off.
void Set(bool bOn);

// Called as each period starts: sidelines slots 2 and 3 in an exhibition match while the mode is
// on. A season or gauntlet match keeps its whole roster, as their saved records expect.
void ParkSeconds();

// Whether the slot is sitting out this period.
bool Parked(int nSlot);

// Before a restart into Two on Two: the sidelined dromers come back healthy.
void Unpark();

} // namespace OneOnOne

#endif
