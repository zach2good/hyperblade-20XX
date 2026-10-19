#include "engine/careerstats.h"
#include "engine/teamblock.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <istream>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>

namespace
{

std::string                        s_strPath;
std::optional<CareerStats::Career> s_career;

// Per side and row, the stats the periods so far have recorded, and whether the match has counted.
std::array<std::array<CareerStats::Stats, kTeamRowCount>, 2> s_aRecorded = {};
std::array<std::array<bool, kTeamRowCount>, 2>               s_abCounted = {};

CareerStats::Career& Loaded()
{
    if (!s_career)
    {
        auto file = std::ifstream(s_strPath);
        s_career  = CareerStats::Parse(file);
    }

    return *s_career;
}

std::string RowKey(const TeamBlock& team, int nRow)
{
    return CareerStats::Key(team.GetName(), team.GetRosterName(kRowRosterSlots[nRow]));
}

void RecordSide(const TeamBlock& team, int nSide)
{
    for (auto nRow = 0; nRow < kTeamRowCount; nRow++)
    {
        auto& recorded = s_aRecorded[nSide][nRow];
        auto  gain     = CareerStats::Stats{};

        for (size_t i = 0; i < gain.size(); i++)
        {
            gain[i]     = team.aStatRows[nRow].aStat[i] - recorded[i];
            recorded[i] = team.aStatRows[nRow].aStat[i];
        }

        const auto bGained = std::ranges::any_of(
            gain,
            [](float flStat)
            {
                return flStat != 0.0f;
            });

        if (bGained)
        {
            CareerStats::AddGain(Loaded(), RowKey(team, nRow), gain, !s_abCounted[nSide][nRow]);
            s_abCounted[nSide][nRow] = true;
        }
    }
}

} // namespace

std::string CareerStats::Key(std::string_view team, std::string_view dromer)
{
    auto strKey = std::string(team);
    strKey += '\t';
    strKey += dromer;

    return strKey;
}

void CareerStats::AddGain(Career& career, std::string_view key, const Stats& gain, bool bFirstOfMatch)
{
    auto it = career.dromers.find(key);

    if (it == career.dromers.end())
    {
        it = career.dromers.emplace(std::string(key), Totals{}).first;
    }

    if (bFirstOfMatch)
    {
        it->second.nMatches++;
    }

    for (size_t i = 0; i < gain.size(); i++)
    {
        it->second.aStat[i] += gain[i];
    }
}

void CareerStats::AddResult(Career& career, std::string_view team, int nScored, int nConceded)
{
    auto it = career.teams.find(team);

    if (it == career.teams.end())
    {
        it = career.teams.emplace(std::string(team), TeamRecord{}).first;
    }

    auto& record = it->second;

    record.nMatches++;
    record.nScored += nScored;
    record.nConceded += nConceded;

    if (nScored > nConceded)
    {
        record.nWins++;
    }
    else if (nScored < nConceded)
    {
        record.nLosses++;
    }
    else
    {
        record.nDraws++;
    }
}

CareerStats::Career CareerStats::Parse(std::istream& in)
{
    auto career  = Career{};
    auto strLine = std::string();

    while (std::getline(in, strLine))
    {
        if (strLine.starts_with('='))
        {
            const auto nNameEnd = strLine.find('\t');

            if (nNameEnd == std::string::npos)
            {
                continue;
            }

            auto record = TeamRecord{};
            auto fields = std::istringstream(strLine.substr(nNameEnd + 1));

            fields >> record.nMatches >> record.nWins >> record.nDraws >> record.nLosses >> record.nScored >> record.nConceded;

            if (!fields.fail())
            {
                career.teams[strLine.substr(1, nNameEnd - 1)] = record;
            }

            continue;
        }

        const auto nTeamEnd = strLine.find('\t');
        const auto nNameEnd = strLine.find('\t', nTeamEnd + 1);

        if (nTeamEnd == std::string::npos || nNameEnd == std::string::npos)
        {
            continue;
        }

        auto totals = Totals{};
        auto fields = std::istringstream(strLine.substr(nNameEnd + 1));

        fields >> totals.nMatches;

        for (auto& flStat : totals.aStat)
        {
            fields >> flStat;
        }

        if (!fields.fail())
        {
            career.dromers[strLine.substr(0, nNameEnd)] = totals;
        }
    }

    return career;
}

void CareerStats::Write(std::ostream& out, const Career& career)
{
    // Enough digits that a float reads back as itself.
    out << std::setprecision(std::numeric_limits<float>::max_digits10);

    for (const auto& [strName, record] : career.teams)
    {
        out << '=' << strName << '\t' << record.nMatches << '\t' << record.nWins << '\t' << record.nDraws << '\t' << record.nLosses << '\t'
            << record.nScored << '\t' << record.nConceded << '\n';
    }

    for (const auto& [strKey, totals] : career.dromers)
    {
        out << strKey << '\t' << totals.nMatches;

        for (const auto flStat : totals.aStat)
        {
            out << '\t' << flStat;
        }

        out << '\n';
    }
}

void CareerStats::Open(std::string strPath)
{
    s_strPath = std::move(strPath);
    s_career.reset();
}

void CareerStats::StartMatch()
{
    s_aRecorded = {};
    s_abCounted = {};
}

void CareerStats::EndPeriod(const TeamBlock& home, const TeamBlock& away, int nHomeScore, int nAwayScore, bool bMatchOver)
{
    if (s_strPath.empty())
    {
        return;
    }

    RecordSide(home, 0);
    RecordSide(away, 1);

    if (bMatchOver)
    {
        AddResult(Loaded(), home.GetName(), nHomeScore, nAwayScore);
        AddResult(Loaded(), away.GetName(), nAwayScore, nHomeScore);
    }

    auto file = std::ofstream(s_strPath, std::ios::trunc);

    Write(file, Loaded());
}

TeamStatRow CareerStats::Row(const TeamBlock& team, int nRow, int* pnMatches)
{
    auto row = TeamStatRow{};

    *pnMatches = 0;

    if (s_strPath.empty())
    {
        return row;
    }

    const auto& dromers = Loaded().dromers;
    const auto  it      = dromers.find(RowKey(team, nRow));

    if (it == dromers.end())
    {
        return row;
    }

    *pnMatches = it->second.nMatches;

    std::ranges::copy(it->second.aStat, row.aStat);

    if (row.aStat[kMatchStatShotsOnGoal] > 0.0f)
    {
        row.aStat[kMatchStatShootingPct] = row.aStat[kMatchStatTotalGoals] / row.aStat[kMatchStatShotsOnGoal] * 100.0f;
    }

    return row;
}

CareerStats::TeamRecord CareerStats::Record(const TeamBlock& team)
{
    if (s_strPath.empty())
    {
        return {};
    }

    const auto& teams = Loaded().teams;
    const auto  it    = teams.find(team.GetName());

    if (it == teams.end())
    {
        return {};
    }

    return it->second;
}
