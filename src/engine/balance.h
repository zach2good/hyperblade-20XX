#ifndef HYPERBLADE_BALANCE_H
#define HYPERBLADE_BALANCE_H

// The balance proposals in docs/sim_results.md, applied to team ratings as a team file loads.
namespace Balance
{

bool On();

// Takes effect when the next match loads its teams.
void Set(bool bOn);

// Brackets the parse of one team file, given its full path.
void BeginTeam(const char* pszPath);
void EndTeam();

// A skater rating as the proposal would have it, or nValue when it leaves this one alone.
int Adjust(const char* pszRating, int nLevel, int nValue);

} // namespace Balance

#endif
