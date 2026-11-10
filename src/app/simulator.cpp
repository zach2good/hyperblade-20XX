#include "common/compat.h"
#include "app/simulator.h"
#include "app/gameapp.h"
#include "app/window.h"
#include "common/clock.h"
#include "common/path.h"
#include "engine/framepacing.h"
#include "engine/game.h"
#include "engine/gameengine.h"
#include "engine/planfixes.h"
#include "engine/spectator.h"
#include "engine/statids.h"
#include "engine/teamblock.h"
#include "engine/teamsblock.h"
#include "globals.h"
#include "math/squirrel5.h"
#include "ui/saveobject.h"
#include "ui/screens/dromeexteriorscreen.h"
#include <SDL3/SDL.h>
#include <adlib/vocabulary.h>
#include <array>
#include <charconv>
#include <cstdlib>
#include <optional>
#include <utility>
#include <cstdio>
#include <print>
#include <fstream>
#include <string>
#include <string_view>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>
#endif

namespace
{

struct Request
{
    std::string strHome;
    std::string strAway;
    int         nCount = 0;
    uint32_t    nSeed  = 1;
    bool        bFixed = true;
    // Beside the executable when empty.
    std::string strOutput;
    // The home team's drome when empty.
    std::string strArena;
    // The first word no argument reads, which stops the run instead of becoming the seed.
    std::string strUnknown;
};

std::optional<Request> s_request;

// A period that runs this long has stalled; the match is dropped.
constexpr auto kMaxPeriodTicks = 30 * 60 * 30;

// Three periods, and a cycle to spare.
constexpr auto kMaxPeriods = 4;

// The roster slots of the four skater rows; slot 2 is the goalie.
// An hour in: nought marks a clock stamp as unset in places.
constexpr auto kSimulatedStart = Clock::Milliseconds{ 60 * 60 * 1000 };

// GameEngine's ARENA_NONE, which makes SetArena load the drome again.
constexpr auto kNoArena = 0xffff;

// Events pumped this often, so the hidden window never looks hung.
constexpr auto kPumpTicks = 600;

// A tournament match is over after its third period; it never sets m_bMatchOver when won.
constexpr auto kTournamentPeriods = 3;

// SaveObject's bracket row ends with this, as GameEngine's kBracketEnd.
constexpr auto kBracketEnd = -1;

// A whole word as a number, or none when any of it is not one.
template <class T>
std::optional<T> ParseWhole(std::string_view word)
{
    auto value            = T{};
    const auto [pEnd, ec] = std::from_chars(word.data(), word.data() + word.size(), value);

    if (ec != std::errc{} || pEnd != word.data() + word.size())
    {
        return std::nullopt;
    }

    return value;
}

// A team by index, file name or name, any case.
std::optional<short> FindTeam(const TeamsBlock& table, std::string_view name)
{
    if (const auto nIndex = ParseWhole<int>(name); nIndex.has_value() && *nIndex >= 0 && *nIndex < table.GetTeamCount())
    {
        return static_cast<short>(*nIndex);
    }

    for (short i = 0; i < table.GetTeamCount(); i++)
    {
        const auto strPath = std::string(table.m_ppTeams[i]->szPath);
        const auto strStem = strPath.substr(0, strPath.find('.'));

        if (SDL_strcasecmp(strStem.c_str(), std::string(name).c_str()) == 0 || SDL_strcasecmp(table.m_ppTeams[i]->szName, std::string(name).c_str()) == 0)
        {
            return i;
        }
    }

    return std::nullopt;
}

void ListTeams(const TeamsBlock& table)
{
    std::println("Teams:");

    for (short i = 0; i < table.GetTeamCount(); i++)
    {
        std::println("  {}  {}  {}", i, table.m_ppTeams[i]->szPath, table.m_ppTeams[i]->szName);
    }
}

// Plays one match from the first period to the last. False when a period stalls.
bool PlayMatch(uint32_t nSeed, Clock::FractionalMilliseconds& elapsed, long long& nTicks)
{
    // A fresh load of both teams and the arena, as loading a save does, so no match inherits the
    // last one's damage or arena state.
    g_pGameEngine->m_bMatchOver  = 1;
    g_pGameEngine->m_nArenaIndex = kNoArena;
    g_pGameEngine->ResetPeriodToFirst();

    for (auto nPeriod = 0; nPeriod < kMaxPeriods; nPeriod++)
    {
        g_nPrevEngineState = kEstateJumbotron;

        Game::SetEngineState(kEstateMatchStart);
        Game::UpdateState();

        // Starting a period picks a random music track, which reseeds the RNG from the clock.
        Math::SeedRand(nSeed * kMaxPeriods + static_cast<uint32_t>(nPeriod));

        auto nPeriodTicks = 0;

        while (g_gameApp.m_nEngineState == kEstateMatchLive)
        {
            if (++nPeriodTicks > kMaxPeriodTicks)
            {
                return false;
            }

            elapsed += Clock::FractionalMilliseconds(FramePacing::kTickMs);

            Clock::g_simulatedNow = std::chrono::duration_cast<Clock::Milliseconds>(elapsed);

            Game::Tick();

            if (nPeriodTicks % kPumpTicks == 0)
            {
                SDL_PumpEvents();
            }
        }

        nTicks += nPeriodTicks;

        if (g_pGameEngine->m_bMatchOver != 0 || (g_pGameEngine->IsTournamentMode() != 0 && nPeriod + 1 >= kTournamentPeriods))
        {
            return true;
        }
    }

    return false;
}

void WriteHeader(std::ofstream& csv, const adlib::Namespace* pStats)
{
    csv << "match,seed,plans,difficulty,period_ms,team,side,player,row,health,score,opponent_score";

    for (auto i = 0; i < kMatchStatCount; i++)
    {
        const auto* const pszName = [&]() -> const char*
        {
            if (pStats != nullptr && pStats->name(i) != nullptr)
            {
                return pStats->name(i);
            }

            return "";
        }();

        if (*pszName != 0)
        {
            csv << ',' << pszName;
        }
        else
        {
            csv << ",stat" << i;
        }
    }

    csv << '\n';
}

void WriteRows(std::ofstream& csv, int nMatch, uint32_t nSeed)
{
    g_pGameEngine->SyncPlayerDataToTeam();

    const auto nHomeScore = g_pGameEngine->GetHomeScore();
    const auto nAwayScore = g_pGameEngine->GetAwayScore();

    const auto* const pszPlans = [&]
    {
        if (PlanFixes::On())
        {
            return "fixed";
        }

        return "shipped";
    }();

    for (auto nSide = 0; nSide < 2; nSide++)
    {
        const auto bHome = nSide == 0;

        const auto* const pTeam = [&]
        {
            if (bHome)
            {
                return g_pGameEngine->m_pTeamA.get();
            }

            return g_pGameEngine->m_pTeamB.get();
        }();

        const auto* const pszSide = [&]
        {
            if (bHome)
            {
                return "home";
            }

            return "away";
        }();

        const auto [nScore, nOpponentScore] = [&]
        {
            if (bHome)
            {
                return std::pair{ nHomeScore, nAwayScore };
            }

            return std::pair{ nAwayScore, nHomeScore };
        }();

        for (auto nRow = 0; nRow < kTeamRowCount; nRow++)
        {
            csv << nMatch << ',' << nSeed << ',' << pszPlans << ',' << static_cast<int>(g_pGameEngine->m_nDifficulty) << ',' << g_pGameEngine->m_periodLength.count() << ',' << pTeam->szName << ',';
            csv << pszSide << ',' << pTeam->GetRosterName(kRowRosterSlots[nRow]) << ',' << nRow << ',';
            csv << pTeam->aRows[nRow].aStat[kRatingDamage] << ',' << nScore << ',' << nOpponentScore;

            for (const auto flStat : pTeam->aStatRows[nRow].aStat)
            {
                csv << ',' << flStat;
            }

            csv << '\n';
        }
    }
}

} // namespace

int Simulator::TakeArguments(int nCount, char** ppszWords)
{
    if (nCount < 3)
    {
        return nCount;
    }

    auto request    = Request{};
    request.strHome = ppszWords[0];
    request.strAway = ppszWords[1];
    request.nCount  = ParseWhole<int>(ppszWords[2]).value_or(0);

    auto nUsed = 3;

    for (; nUsed < nCount && nUsed < 7 && ppszWords[nUsed][0] != '-'; nUsed++)
    {
        const auto word = std::string_view(ppszWords[nUsed]);

        if (word == "shipped" || word == "fixed")
        {
            request.bFixed = word == "fixed";
        }
        else if (word.ends_with(".csv"))
        {
            request.strOutput = word;
        }
        else if (word.starts_with("arena="))
        {
            request.strArena = word.substr(6);
        }
        else if (const auto nSeed = ParseWhole<uint32_t>(word); nSeed.has_value())
        {
            request.nSeed = *nSeed;
        }
        else if (request.strUnknown.empty())
        {
            request.strUnknown = word;
        }
    }

    s_request = request;

    return nUsed;
}

bool Simulator::Requested()
{
    return s_request.has_value();
}

// Each season starts a Gauntlet as the Tournament setup screen does and plays its bracket out.
// Returns the matches played.
int PlayGauntlet(const Request& request, std::ofstream& csv, short nHome, Clock::FractionalMilliseconds& elapsed, long long& nTicks)
{
    auto* const pSave  = g_pGameEngine->m_pSaveObject.get();
    const auto  nTeams = g_pGameEngine->m_pTeamTable->GetTeamCount();

    auto nMatch  = 0;
    auto nTitles = 0;

    for (auto nSeason = 0; nSeason < request.nCount; nSeason++)
    {
        g_nPlayerMode = kPlayerModeTournament;

        pSave->ValidateSetHomeTeam(nHome, 1);
        pSave->BracketIterReset();

        auto nOpponent = static_cast<int>(pSave->BracketIterPeek());

        const auto* const pszResult = [&]
        {
            for (auto nRound = 1; nRound <= nTeams; nRound++)
            {
                const auto nSeed = request.nSeed + static_cast<uint32_t>(nMatch);

                g_nDromeTeamAway = nOpponent;
                g_nDromeArena    = nOpponent;

                if (!PlayMatch(nSeed, elapsed, nTicks))
                {
                    return "stalled";
                }

                WriteRows(csv, nMatch, nSeed);
                csv.flush();
                nMatch++;

                if (g_nDromeTeamAway == kBracketEnd)
                {
                    return "won";
                }

                if (pSave->GetBracketPos() == 0)
                {
                    const auto nHome = static_cast<int>(g_pGameEngine->GetHomeScore());
                    const auto nAway = static_cast<int>(g_pGameEngine->GetAwayScore());

                    // A draw ends a season too, unless every opponent is down.
                    const auto* const pszHow = [&]
                    {
                        if (nHome == nAway)
                        {
                            return "drew with";
                        }

                        return "lost to";
                    }();

                    std::println("season {}: {} {} in round {}, {}-{}", nSeason, pszHow, g_pGameEngine->m_pTeamB->szName, nRound, nHome, nAway);

                    return "lost";
                }

                nOpponent = g_nDromeTeamAway;
            }

            return "unfinished";
        }();

        if (SDL_strcmp(pszResult, "won") == 0)
        {
            nTitles++;
            std::println("season {}: won the Gauntlet", nSeason);
        }
        else if (SDL_strcmp(pszResult, "lost") != 0)
        {
            std::println("season {}: {}", nSeason, pszResult);
        }
    }

    std::println("{} titles in {} seasons", nTitles, request.nCount);

    return nMatch;
}

int Simulator::Run()
{
    if (!s_request.has_value())
    {
        return 2;
    }

    const auto& request = *s_request;

#if defined(_WIN32)
    // A windowed program started from a console has no stdout unless it was redirected; it
    // reports to that console instead.
    if (_fileno(stdout) < 0 && AttachConsole(ATTACH_PARENT_PROCESS))
    {
        std::freopen("CONOUT$", "w", stdout);
    }
#endif

    SDL_HideWindow(Window::Native().As<SDL_Window>());

    const auto& table     = *g_pGameEngine->m_pTeamTable;
    const auto  bGauntlet = SDL_strcasecmp(request.strAway.c_str(), "gauntlet") == 0;
    const auto  home      = FindTeam(table, request.strHome);

    // A Gauntlet's opponents come from the bracket, so any team stands in for the away one.
    const auto away = [&]
    {
        if (bGauntlet)
        {
            return home;
        }

        return FindTeam(table, request.strAway);
    }();

    // A team's drome is the arena of the same index.
    const auto arena = [&]
    {
        if (request.strArena.empty())
        {
            return home;
        }

        return FindTeam(table, request.strArena);
    }();

    if (!request.strUnknown.empty())
    {
        std::println("unknown argument: {}", request.strUnknown);
    }

    if (!home.has_value() || !away.has_value() || !arena.has_value() || request.nCount <= 0 || !request.strUnknown.empty())
    {
        std::println("usage: hyperblade_20XX -sim HOME AWAY|gauntlet COUNT [SEED] [shipped|fixed] [arena=TEAM] [OUTPUT.csv]");
        ListTeams(table);

        return 2;
    }

    auto vocabulary = adlib::Vocabulary();
    vocabulary.loadEnumHeaderFile(Path::Native(String::Concat(g_gameApp.m_strExeDir.m_str, "ENUMIDS.H").m_pchData));

    const auto strCsv = [&]
    {
        if (!request.strOutput.empty())
        {
            return request.strOutput;
        }

        return std::string(SDL_GetBasePath()) + "sim_results.csv";
    }();
    // Binary, so every platform writes the same line endings.
    auto csv = std::ofstream(strCsv, std::ios::binary | std::ios::trunc);

    WriteHeader(csv, vocabulary.find("StatID"));

    Spectator::SetAutopilot(true);
    PlanFixes::Set(request.bFixed);

    // A fixed start, so no timer depends on when the run began.
    Clock::g_bSimulated   = true;
    Clock::g_simulatedNow = kSimulatedStart;

    auto elapsed = Clock::FractionalMilliseconds(Clock::g_simulatedNow);

    const auto start  = std::chrono::steady_clock::now();
    auto       nTicks = 0LL;
    auto       nDone  = 0;

    if (bGauntlet)
    {
        nDone = PlayGauntlet(request, csv, *home, elapsed, nTicks);
    }

    for (auto nMatch = 0; !bGauntlet && nMatch < request.nCount; nMatch++)
    {
        const auto nSeed = request.nSeed + static_cast<uint32_t>(nMatch);

        g_nPlayerMode    = kPlayerModeExhibition;
        g_nDromeTeamHome = *home;
        g_nDromeTeamAway = *away;
        g_nDromeArena    = *arena;

        if (!PlayMatch(nSeed, elapsed, nTicks))
        {
            std::println("match {} (seed {}) stalled and was dropped", nMatch, nSeed);

            continue;
        }

        WriteRows(csv, nMatch, nSeed);

        // A fatal error ends the process, and the matches before it should survive.
        csv.flush();
        nDone++;

        std::println("match {}: {} {}, {} {}", nMatch, g_pGameEngine->m_pTeamA->szName, g_pGameEngine->GetHomeScore(), g_pGameEngine->m_pTeamB->szName, g_pGameEngine->GetAwayScore());
    }

    const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    const auto nPlanned = [&]
    {
        // A Gauntlet's count is seasons, and every match it started is counted.
        if (bGauntlet)
        {
            return nDone;
        }

        return request.nCount;
    }();

    std::println("{} of {} matches, {} ticks in {:.1f} s: {:.0f} ticks a second, {:.0f}x real time\n{}",
                 nDone,
                 nPlanned,
                 nTicks,
                 seconds,
                 static_cast<double>(nTicks) / seconds,
                 static_cast<double>(nTicks) * FramePacing::kTickMs / (seconds * 1000.0),
                 strCsv);

    return 0;
}
