// Adds matches to a career table and round-trips it through the text format.

#include "engine/careerstats.h"
#include <cstdio>
#include <sstream>

static int s_nFailed = 0;

static void Expect(bool bOK, const char* pszWhat)
{
    if (!bOK)
    {
        std::printf("failed: %s\n", pszWhat);
        s_nFailed++;
    }
}

int main()
{
    auto table = CareerStats::Table{};
    auto stats = std::array<float, kMatchStatCount>{};

    const auto key = CareerStats::Key("The Knights", "Al Smith");

    CareerStats::AddMatch(table, key, stats);
    Expect(table.empty(), "an all-nought row is a reserve and adds nothing");

    stats[kMatchStatTotalGoals]  = 2.0f;
    stats[kMatchStatShotsOnGoal] = 5.0f;
    stats[kMatchStatTimeOfPoss]  = 6125041.5f;

    CareerStats::AddMatch(table, key, stats);
    CareerStats::AddMatch(table, key, stats);

    Expect(table.at(key).nMatches == 2, "two matches counted");
    Expect(table.at(key).aStat[kMatchStatTotalGoals] == 4.0f, "goals summed");

    auto out = std::stringstream();
    CareerStats::Write(out, table);

    auto       in   = std::stringstream(out.str() + "a damaged line\n");
    const auto read = CareerStats::Parse(in);

    Expect(read.size() == 1, "the damaged line is skipped");
    Expect(read.contains(key), "the name with a space survives");
    Expect(read.at(key).nMatches == 2, "the match count survives");
    Expect(read.at(key).aStat[kMatchStatTimeOfPoss] == 12250083.0f, "the sums survive");

    return s_nFailed;
}
