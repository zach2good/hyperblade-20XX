#include "engine/balance.h"
#include "common/fileobj.h"
#include "engine/planfixes.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

// BLAM! Nemesis moves halfway to The Machine's median for each rating and difficulty.
constexpr auto kShrink = 0.5;

constexpr auto kLevels = 3;

using Medians = std::map<std::pair<int, std::string>, double>;

bool s_bOn = false;

std::optional<Medians> s_machine;

const Medians* s_pActive = nullptr;

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

// Every `name = value` on the file's SetRatings1 to SetRatings3 lines, by level and rating.
std::optional<Medians> ReadMedians(const std::string& strPath)
{
    auto file = FileObj();

    if (file.Open(strPath.c_str(), FileMode::kRead) == 0)
    {
        return std::nullopt;
    }

    auto values = std::map<std::pair<int, std::string>, std::vector<int> >();

    char* pszLine = nullptr;

    while (file.GetNextLine(&pszLine) > 0)
    {
        if (pszLine == nullptr)
        {
            continue;
        }

        auto       line   = std::string_view(pszLine);
        const auto nStart = std::min(line.find_first_not_of(" \t"), line.size());

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

            values[{ nLevel, Key(name) }].push_back(nValue);

            line.remove_prefix(static_cast<size_t>(result.ptr - line.data()));
        }
    }

    file.Close();

    auto medians = Medians();

    for (auto& [key, list] : values)
    {
        std::ranges::sort(list);

        const auto nMiddle = list.size() / 2;

        medians[key] = [&]
        {
            if (list.size() % 2 == 0)
            {
                return (list[nMiddle - 1] + list[nMiddle]) / 2.0;
            }

            return static_cast<double>(list[nMiddle]);
        }();
    }

    return medians;
}

} // namespace

bool Balance::On()
{
    return s_bOn;
}

void Balance::Set(bool bOn)
{
    s_bOn = bOn;
}

void Balance::BeginTeam(const char* pszPath)
{
    s_pActive = nullptr;

    const auto path = std::string_view(pszPath);

    if (!s_bOn || SDL_strcasecmp(std::string(FileName(path)).c_str(), "NEM.HTM") != 0)
    {
        return;
    }

    if (!s_machine.has_value())
    {
        s_machine = ReadMedians(std::string(path.substr(0, path.size() - FileName(path).size())) + "CHICO.HTM");
    }

    if (s_machine.has_value())
    {
        s_pActive = &*s_machine;
    }
}

void Balance::EndTeam()
{
    s_pActive = nullptr;
}

int Balance::Adjust(const char* pszRating, int nLevel, int nValue)
{
    if (s_pActive == nullptr)
    {
        return nValue;
    }

    const auto found = s_pActive->find({ nLevel, Key(pszRating) });

    if (found == s_pActive->end())
    {
        return nValue;
    }

    const auto flMedian = found->second;

    // Halves round to even, as the measured proposal did.
    return static_cast<int>(std::nearbyint(flMedian + kShrink * (nValue - flMedian)));
}
