#include "engine/balance.h"
#include "common/fileobj.h"
#include "engine/gameoptions.h"
#include "engine/planfixes.h"
#include "globals.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace
{

constexpr auto kLevels = 3;

// What a team's rating moves toward: The Machine's median for that rating and difficulty, or
// The Machine's own value for the same dromer, rating and difficulty.
enum class Target
{
    Median,
    SameDromer,
};

struct Proposal
{
    const char* pszFile;
    Target      target;
    double      flShrink;
};

constexpr Proposal kProposals[] = {
    { "NEM.HTM", Target::Median, 0.6875 },
    { "MUNIC.HTM", Target::SameDromer, 0.375 },
};

// The Machine's ratings, by (level, rating) and by (dromer, level, rating).
struct Machine
{
    std::map<std::pair<int, std::string>, double>    medians;
    std::map<std::tuple<int, int, std::string>, int> values;
};

constexpr auto kSection = "Gameplay";
constexpr auto kKey     = "BalancedTeams";

std::optional<bool> s_obOn;

std::optional<Machine> s_machine;

const Proposal* s_pProposal = nullptr;

// The definition Adjust last saw, and how many have come with ratings so far.
const void* s_pDromer = nullptr;
int         s_nDromer = -1;

std::string_view FileName(std::string_view path)
{
    const auto nSlash = path.find_last_of("/\\");

    if (nSlash == std::string_view::npos)
    {
        return path;
    }

    return path.substr(nSlash + 1);
}

std::string Key(std::string_view name)
{
    for (const auto& [pszAlias, pszName] : PlanFixes::kRatingAliases)
    {
        if (name.size() == SDL_strlen(pszAlias) && SDL_strncasecmp(name.data(), pszAlias, name.size()) == 0)
        {
            name = pszName;
        }
    }

    auto key = std::string(name);

    std::ranges::transform(key, key.begin(), [](unsigned char c)
                           {
                               return static_cast<char>(SDL_tolower(c));
                           });

    return key;
}

// Every `name = value` on the file's SetRatings1 to SetRatings3 lines. A dromer is a definition
// with such lines, counted from 0.
std::optional<Machine> ReadMachine(const std::string& strPath)
{
    auto file = FileObj();

    if (file.Open(strPath.c_str(), FileMode::kRead) == 0)
    {
        return std::nullopt;
    }

    auto machine = Machine();
    auto lists   = std::map<std::pair<int, std::string>, std::vector<int> >();
    auto nDromer = -1;
    auto bNewDef = false;

    char* pszLine = nullptr;

    while (file.GetNextLine(&pszLine) > 0)
    {
        if (pszLine == nullptr)
        {
            continue;
        }

        auto       line   = std::string_view(pszLine);
        const auto nStart = std::min(line.find_first_not_of(" \t"), line.size());

        constexpr char kDefine[] = "DefineObject";

        if (SDL_strncasecmp(pszLine + nStart, kDefine, sizeof(kDefine) - 1) == 0)
        {
            bNewDef = true;
            continue;
        }

        constexpr char kCommand[]   = "SetRatings";
        constexpr auto nCommandSize = sizeof(kCommand) - 1;

        if (line.size() - nStart <= nCommandSize || SDL_strncasecmp(pszLine + nStart, kCommand, nCommandSize) != 0)
        {
            continue;
        }

        line.remove_prefix(nStart);

        const auto nLevel = line[nCommandSize] - '1';

        if (nLevel < 0 || nLevel >= kLevels)
        {
            continue;
        }

        line.remove_prefix(nCommandSize + 1);

        if (bNewDef)
        {
            bNewDef = false;
            nDromer++;
        }

        for (auto nEquals = line.find('='); nEquals != std::string_view::npos; nEquals = line.find('='))
        {
            auto name = line.substr(0, nEquals);

            name.remove_suffix(name.size() - (name.find_last_not_of(" \t") + 1));
            name.remove_prefix(std::min(name.find_last_of(" \t") + 1, name.size()));

            line.remove_prefix(nEquals + 1);
            line.remove_prefix(std::min(line.find_first_not_of(" \t"), line.size()));

            auto       nValue = 0;
            const auto result = std::from_chars(line.data(), line.data() + line.size(), nValue);

            if (result.ec != std::errc())
            {
                break;
            }

            auto key = Key(name);

            lists[{ nLevel, key }].push_back(nValue);
            machine.values[{ nDromer, nLevel, std::move(key) }] = nValue;

            line.remove_prefix(static_cast<size_t>(result.ptr - line.data()));
        }
    }

    file.Close();

    for (auto& [key, list] : lists)
    {
        std::ranges::sort(list);

        const auto nMiddle = list.size() / 2;

        machine.medians[key] = [&]
        {
            if (list.size() % 2 == 0)
            {
                return (list[nMiddle - 1] + list[nMiddle]) / 2.0;
            }

            return static_cast<double>(list[nMiddle]);
        }();
    }

    return machine;
}

} // namespace

bool Balance::On()
{
    if (!s_obOn.has_value() && g_pGameOptions != nullptr)
    {
        s_obOn = g_pGameOptions->ReadInt(kSection, kKey, 0) != 0;
    }

    return s_obOn.value_or(false);
}

void Balance::Set(bool bOn)
{
    s_obOn = bOn;
}

void Balance::Save()
{
    if (g_pGameOptions != nullptr)
    {
        g_pGameOptions->WriteInt(kSection, kKey, static_cast<int>(On()));
    }
}

void Balance::BeginTeam(const char* pszPath)
{
    EndTeam();

    if (!On())
    {
        return;
    }

    const auto path = std::string_view(pszPath);
    const auto name = std::string(FileName(path));

    const auto* const pProposal = std::ranges::find_if(
        kProposals,
        [&](const Proposal& proposal)
        {
            return SDL_strcasecmp(name.c_str(), proposal.pszFile) == 0;
        });

    if (pProposal == std::end(kProposals))
    {
        return;
    }

    if (!s_machine.has_value())
    {
        s_machine = ReadMachine(std::string(path.substr(0, path.size() - name.size())) + "CHICO.HTM");
    }

    if (s_machine.has_value())
    {
        s_pProposal = pProposal;
    }
}

void Balance::EndTeam()
{
    s_pProposal = nullptr;
    s_pDromer   = nullptr;
    s_nDromer   = -1;
}

int Balance::Adjust(const void* pDromer, const char* pszRating, int nLevel, int nValue)
{
    if (s_pProposal == nullptr)
    {
        return nValue;
    }

    if (pDromer != s_pDromer)
    {
        s_pDromer = pDromer;
        s_nDromer++;
    }

    const auto key = Key(pszRating);

    const auto target = [&] -> std::optional<double>
    {
        if (s_pProposal->target == Target::Median)
        {
            const auto found = s_machine->medians.find({ nLevel, key });

            if (found == s_machine->medians.end())
            {
                return std::nullopt;
            }

            return found->second;
        }

        const auto found = s_machine->values.find({ s_nDromer, nLevel, key });

        if (found == s_machine->values.end())
        {
            return std::nullopt;
        }

        return found->second;
    }();

    if (!target.has_value())
    {
        return nValue;
    }

    // Halves round to even, as the measured proposals did.
    return static_cast<int>(std::nearbyint(*target + s_pProposal->flShrink * (nValue - *target)));
}
