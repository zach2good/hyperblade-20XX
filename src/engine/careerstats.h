// Each dromer's and each team's lifetime stats over every exhibition and Gauntlet match, kept in a
// text file beside the executable and written as each period ends. The original keeps one match's
// stats and clears them at the next.

#ifndef HYPERBLADE_CAREERSTATS_H
#define HYPERBLADE_CAREERSTATS_H

#include "engine/statids.h"
#include <array>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>

struct TeamBlock;
struct TeamStatRow;

namespace CareerStats
{

using Stats = std::array<float, kMatchStatCount>;

struct Totals
{
    int   nMatches = 0;
    Stats aStat    = {};
};

struct TeamRecord
{
    int nMatches  = 0;
    int nWins     = 0;
    int nDraws    = 0;
    int nLosses   = 0;
    int nScored   = 0;
    int nConceded = 0;
};

struct Career
{
    // Keyed by team name and dromer name, a tab between them.
    std::map<std::string, Totals, std::less<> >     dromers;
    std::map<std::string, TeamRecord, std::less<> > teams;
};

std::string Key(std::string_view team, std::string_view dromer);

// Adds what a dromer gained in a period, counting the match on the first gain of it. A period
// with no gain adds nothing, so a reserve who never comes on plays no matches.
void AddGain(Career& career, std::string_view key, const Stats& gain, bool bFirstOfMatch);

void AddResult(Career& career, std::string_view team, int nScored, int nConceded);

// A dromer's line is the key, the match count and the 49 sums; a team's starts with '=' and its
// name, then the six counts. Fields are tab separated.
Career Parse(std::istream& in);
void   Write(std::ostream& out, const Career& career);

// Names the file; it is read on first use. Until then nothing records.
void Open(std::string strPath);

// Forgets what the last match recorded, so the next period counts from nought.
void StartMatch();

// Adds each dromer's gain since the last period, the result too once the match is over, and saves.
void EndPeriod(const TeamBlock& home, const TeamBlock& away, int nHomeScore, int nAwayScore, bool bMatchOver);

// The row's lifetime sums, with the shooting percentage worked out from them, and its match count.
TeamStatRow Row(const TeamBlock& team, int nRow, int* pnMatches);
TeamRecord  Record(const TeamBlock& team);

} // namespace CareerStats

#endif
