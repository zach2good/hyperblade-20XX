#include "common/path.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string_view>

#ifndef _WIN32
namespace
{

bool SameName(std::string_view first, std::string_view second)
{
    return std::ranges::equal(
        first,
        second,
        [](unsigned char a, unsigned char b)
        {
            return std::tolower(a) == std::tolower(b);
        });
}

// The entry in a directory that matches a name without regard to case, or the name itself.
std::filesystem::path MatchEntry(const std::filesystem::path& directory, const std::filesystem::path& name)
{
    auto error = std::error_code();

    for (auto it = std::filesystem::directory_iterator(directory, error); !error && it != std::filesystem::directory_iterator(); it.increment(error))
    {
        const auto candidate = it->path().filename();

        if (SameName(candidate.native(), name.native()))
        {
            return candidate;
        }
    }

    return name;
}

} // namespace
#endif

std::string Path::Native(const char* pszPath)
{
    auto text = std::string(pszPath);

    std::ranges::replace(text, '\\', '/');

#ifdef _WIN32
    // Windows already matches names without regard to case.
    return text;
#else
    const auto path = std::filesystem::path(text);

    if (auto error = std::error_code(); std::filesystem::exists(path, error))
    {
        return text;
    }

    auto resolved = path.root_path();

    for (const auto& part : path.relative_path())
    {
        const auto directory = [&]
        {
            if (resolved.empty())
            {
                return std::filesystem::path(".");
            }

            return resolved;
        }();

        resolved /= MatchEntry(directory, part);
    }

    return resolved.generic_string();
#endif
}
