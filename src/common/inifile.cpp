#include "inifile.h"
#include "common/path.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace
{

std::string_view Trim(std::string_view text)
{
    const auto nStart = text.find_first_not_of(" \t\r");

    if (nStart == std::string_view::npos)
    {
        return {};
    }

    const auto nEnd = text.find_last_not_of(" \t\r");

    return text.substr(nStart, nEnd - nStart + 1);
}

bool SameName(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y)
                                              {
                                                  return std::tolower((unsigned char)x) == std::tolower((unsigned char)y);
                                              });
}

// The name inside "[name]", or nothing for a line that is not a section header.
std::optional<std::string_view> SectionName(std::string_view line)
{
    const auto trimmed = Trim(line);

    if (trimmed.size() < 2 || trimmed.front() != '[' || trimmed.back() != ']')
    {
        return std::nullopt;
    }

    return Trim(trimmed.substr(1, trimmed.size() - 2));
}

std::optional<std::string_view> KeyName(std::string_view line)
{
    const auto nEquals = line.find('=');

    if (nEquals == std::string_view::npos)
    {
        return std::nullopt;
    }

    return Trim(line.substr(0, nEquals));
}

std::vector<std::string> ReadLines(const char* pszPath)
{
    auto lines = std::vector<std::string>();
    auto file  = std::ifstream(Path::Native(pszPath));
    auto line  = std::string();

    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        lines.push_back(line);
    }

    return lines;
}

struct Found
{
    std::optional<size_t> nSection;
    std::optional<size_t> nKey;
    size_t                nSectionEnd = 0;
};

Found Find(const std::vector<std::string>& lines, std::string_view section, std::string_view key)
{
    auto found = Found{};

    for (size_t nLine = 0; nLine < lines.size(); nLine++)
    {
        const auto name = SectionName(lines[nLine]);

        if (name)
        {
            if (found.nSection)
            {
                break;
            }

            if (SameName(*name, section))
            {
                found.nSection    = nLine;
                found.nSectionEnd = nLine + 1;
            }

            continue;
        }

        if (!found.nSection)
        {
            continue;
        }

        if (!Trim(lines[nLine]).empty())
        {
            found.nSectionEnd = nLine + 1;
        }

        const auto keyName = KeyName(lines[nLine]);

        if (keyName && SameName(*keyName, key))
        {
            found.nKey = nLine;

            break;
        }
    }

    return found;
}

std::optional<std::string> ReadValue(const char* pszPath, const char* pszSection, const char* pszKey)
{
    const auto lines = ReadLines(pszPath);
    const auto found = Find(lines, pszSection, pszKey);

    if (!found.nKey)
    {
        return std::nullopt;
    }

    const auto& line = lines[*found.nKey];

    return std::string(Trim(std::string_view(line).substr(line.find('=') + 1)));
}

} // namespace

// FUNCTION: HYPERX 0x00479960
std::string IniFile::ReadString(const char* pszPath, const char* pszSection, const char* pszKey, const char* pszDefault)
{
    return ReadValue(pszPath, pszSection, pszKey).value_or(pszDefault);
}

unsigned int IniFile::ReadInt(const char* pszPath, const char* pszSection, const char* pszKey, int nDefault)
{
    const auto value = ReadValue(pszPath, pszSection, pszKey);

    if (!value)
    {
        return (unsigned int)nDefault;
    }

    const auto nRead = std::strtol(value->c_str(), nullptr, 10);

    if (nRead < 0)
    {
        return 0;
    }

    return (unsigned int)nRead;
}

// FUNCTION: HYPERX 0x004798d0
bool IniFile::WriteString(const char* pszPath, const char* pszSection, const char* pszKey, const char* pszValue)
{
    auto       lines = ReadLines(pszPath);
    const auto found = Find(lines, pszSection, pszKey);
    const auto entry = std::string(pszKey) + "=" + pszValue;

    if (found.nKey)
    {
        lines[*found.nKey] = entry;
    }
    else if (found.nSection)
    {
        lines.insert(lines.begin() + (std::ptrdiff_t)found.nSectionEnd, entry);
    }
    else
    {
        if (!lines.empty() && !Trim(lines.back()).empty())
        {
            lines.emplace_back();
        }

        lines.push_back(std::string("[") + pszSection + "]");
        lines.push_back(entry);
    }

    auto file = std::ofstream(Path::Native(pszPath), std::ios::binary | std::ios::trunc);

    for (const auto& line : lines)
    {
        file << line << "\r\n";
    }

    return file.good();
}
