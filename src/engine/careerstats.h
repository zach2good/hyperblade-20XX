// Each player's totals over every Gauntlet match, kept in a text file beside the executable. The
// original keeps one match's stats and clears them at the next.

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

struct Totals
{
    int                                nMatches = 0;
    std::array<float, kMatchStatCount> aStat    = {};
};

// Keyed by team name and player name, a tab between them.
using Table = std::map<std::string, Totals, std::less<> >;

std::string Key(std::string_view team, std::string_view player);

// Adds one match, unless every stat is nought: a reserve who never came on.
void AddMatch(Table& table, std::string_view key, const std::array<float, kMatchStatCount>& stats);

// One line a player: the key, the match count and the 49 sums, all tab separated.
Table Parse(std::istream& in);
void  Write(std::ostream& out, const Table& table);

// Names the file; it is read on first use.
void Open(std::string strPath);

// Adds the match just played for every row of the team. Save writes the file.
void RecordMatch(const TeamBlock& team);
void Save();

// The row's career sums, with the shooting percentage worked out from them, and its match count.
TeamStatRow Row(const TeamBlock& team, int nRow, int* pnMatches);

} // namespace CareerStats

#endif
