// Adds period gains and results to a career and round-trips it through the text format.

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
    auto career = CareerStats::Career{};
    auto gain   = CareerStats::Stats{};

    const auto key = CareerStats::Key("The Knights", "Al Smith");

    gain[kMatchStatTotalGoals]  = 2.0f;
    gain[kMatchStatShotsOnGoal] = 5.0f;
    gain[kMatchStatTimeOfPoss]  = 1000041.5f;

    // Two periods of one match, then the first period of another.
    CareerStats::AddGain(career, key, gain, true);
    CareerStats::AddGain(career, key, gain, false);
    CareerStats::AddGain(career, key, gain, true);

    Expect(career.dromers.at(key).nMatches == 2, "a match counts once however many periods add to it");
    Expect(career.dromers.at(key).aStat[kMatchStatTotalGoals] == 6.0f, "every period's goals summed");

    CareerStats::AddResult(career, "The Knights", 9, 4);
    CareerStats::AddResult(career, "The Knights", 3, 3);
    CareerStats::AddResult(career, "BLAM! Nemesis", 2, 7);

    const auto& knights = career.teams.at("The Knights");

    Expect(knights.nMatches == 2 && knights.nWins == 1 && knights.nDraws == 1 && knights.nLosses == 0, "wins and draws counted");
    Expect(knights.nScored == 12 && knights.nConceded == 7, "points summed");

    auto out = std::stringstream();
    CareerStats::Write(out, career);

    auto       in   = std::stringstream(out.str() + "a damaged line\n=Also damaged\n");
    const auto read = CareerStats::Parse(in);

    Expect(read.dromers.size() == 1 && read.teams.size() == 2, "the damaged lines are skipped");
    Expect(read.dromers.contains(key), "the name with a space survives");
    Expect(read.dromers.at(key).nMatches == 2, "the match count survives");
    Expect(read.dromers.at(key).aStat[kMatchStatTimeOfPoss] == 3000124.5f, "the sums survive at full precision");
    Expect(read.teams.at("BLAM! Nemesis").nLosses == 1, "the team record survives");

    return s_nFailed;
}
