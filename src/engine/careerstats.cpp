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

std::string                       s_strPath;
std::optional<CareerStats::Table> s_table;

CareerStats::Table& Loaded()
{
    if (!s_table)
    {
        auto file = std::ifstream(s_strPath);
        s_table   = CareerStats::Parse(file);
    }

    return *s_table;
}

std::string RowKey(const TeamBlock& team, int nRow)
{
    return CareerStats::Key(team.GetName(), team.GetRosterName(kRowRosterSlots[nRow]));
}

} // namespace

std::string CareerStats::Key(std::string_view team, std::string_view player)
{
    auto strKey = std::string(team);
    strKey += '\t';
    strKey += player;

    return strKey;
}

void CareerStats::AddMatch(Table& table, std::string_view key, const std::array<float, kMatchStatCount>& stats)
{
    const auto bReserve = std::ranges::all_of(
        stats,
        [](float flStat)
        {
            return flStat == 0.0f;
        });

    if (bReserve)
    {
        return;
    }

    auto it = table.find(key);

    if (it == table.end())
    {
        it = table.emplace(std::string(key), Totals{}).first;
    }

    it->second.nMatches++;

    for (size_t i = 0; i < stats.size(); i++)
    {
        it->second.aStat[i] += stats[i];
    }
}

CareerStats::Table CareerStats::Parse(std::istream& in)
{
    auto table   = Table{};
    auto strLine = std::string();

    while (std::getline(in, strLine))
    {
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
            table[strLine.substr(0, nNameEnd)] = totals;
        }
    }

    return table;
}

void CareerStats::Write(std::ostream& out, const Table& table)
{
    // Enough digits that a float reads back as itself.
    out << std::setprecision(std::numeric_limits<float>::max_digits10);

    for (const auto& [strKey, totals] : table)
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
    s_table.reset();
}

void CareerStats::RecordMatch(const TeamBlock& team)
{
    for (auto nRow = 0; nRow < kTeamRowCount; nRow++)
    {
        auto stats = std::array<float, kMatchStatCount>{};

        std::ranges::copy(team.aStatRows[nRow].aStat, stats.begin());

        AddMatch(Loaded(), RowKey(team, nRow), stats);
    }
}

void CareerStats::Save()
{
    auto file = std::ofstream(s_strPath, std::ios::trunc);

    Write(file, Loaded());
}

TeamStatRow CareerStats::Row(const TeamBlock& team, int nRow, int* pnMatches)
{
    auto row = TeamStatRow{};

    *pnMatches = 0;

    const auto& table = Loaded();
    const auto  it    = table.find(RowKey(team, nRow));

    if (it == table.end())
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
