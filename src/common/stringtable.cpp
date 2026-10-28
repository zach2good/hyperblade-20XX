#include "stringtable.h"
#include "common/path.h"
#include "common/peresources.h"
#include "app/stringids.h"
#include <cstdio>
#include <array>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace
{

// The game's ids stop well short of this.
constexpr auto kMaxStringID = 0x1000u;

// The original pointed players at a joystick control panel and a DOS set-up tool.
constexpr auto s_aOverrides = std::array{
    std::pair{ kStringJoystickControlPanel, "Plug in an Xbox, PlayStation or Switch controller at any time.\n\n"
                                            "Skate with the left stick or d-pad. A attacks, LB is a speed burst, X shoots and Y passes. "
                                            "Start pauses.\n\nIn the menus, A selects and B goes back. " },
    std::pair{ kStringGamePadHelp, "The [Gamepad] section of booie.ini picks the action for each button and trigger." },
};

std::unordered_map<unsigned int, std::string> s_strings;

std::string Escape(std::string_view text)
{
    auto escaped = std::string();

    for (const char c : text)
    {
        switch (c)
        {
            case '\\':
                escaped += "\\\\";
                break;

            case '\n':
                escaped += "\\n";
                break;

            case '\t':
                escaped += "\\t";
                break;

            case '\r':
                escaped += "\\r";
                break;

            default:
                escaped += c;
                break;
        }
    }

    return escaped;
}

std::string Unescape(std::string_view text)
{
    auto plain = std::string();

    for (size_t i = 0; i < text.size(); i++)
    {
        if (text[i] != '\\' || i + 1 == text.size())
        {
            plain += text[i];

            continue;
        }

        i++;

        switch (text[i])
        {
            case 'n':
                plain += '\n';
                break;

            case 't':
                plain += '\t';
                break;

            case 'r':
                plain += '\r';
                break;

            default:
                plain += text[i];
                break;
        }
    }

    return plain;
}

// The file is UTF-8 and the game's text drawing reads each byte as Latin-1. Characters beyond it
// draw as a question mark.
std::string ToGameText(const std::string& utf8)
{
    auto text = std::string();

    for (size_t i = 0; i < utf8.size();)
    {
        const auto nLead = static_cast<unsigned char>(utf8[i]);

        const auto nLength = [&]
        {
            if (nLead < 0x80)
            {
                return 1;
            }

            if ((nLead & 0xe0) == 0xc0)
            {
                return 2;
            }

            if ((nLead & 0xf0) == 0xe0)
            {
                return 3;
            }

            return 4;
        }();

        auto nCode = static_cast<unsigned int>(nLead & (0x7f >> nLength));

        for (int n = 1; n < nLength && i + n < utf8.size(); n++)
        {
            nCode = nCode << 6 | (static_cast<unsigned char>(utf8[i + n]) & 0x3f);
        }

        if (nLength == 1)
        {
            nCode = nLead;
        }

        text += [&]
        {
            if (nCode < 0x100)
            {
                return static_cast<char>(nCode);
            }

            return '?';
        }();
        i += nLength;
    }

    return text;
}

void AppendUTF8(std::string& out, unsigned int nCode)
{
    if (nCode < 0x80)
    {
        out += static_cast<char>(nCode);
    }
    else if (nCode < 0x800)
    {
        out += static_cast<char>(0xc0 | nCode >> 6);
        out += static_cast<char>(0x80 | (nCode & 0x3f));
    }
    else if (nCode < 0x10000)
    {
        out += static_cast<char>(0xe0 | nCode >> 12);
        out += static_cast<char>(0x80 | (nCode >> 6 & 0x3f));
        out += static_cast<char>(0x80 | (nCode & 0x3f));
    }
    else
    {
        out += static_cast<char>(0xf0 | nCode >> 18);
        out += static_cast<char>(0x80 | (nCode >> 12 & 0x3f));
        out += static_cast<char>(0x80 | (nCode >> 6 & 0x3f));
        out += static_cast<char>(0x80 | (nCode & 0x3f));
    }
}

// The program's string resources. Under RT_STRING, block n holds ids (n - 1) * 16 onward as
// sixteen counted UTF-16 strings.
std::optional<std::map<unsigned int, std::string> > ReadStringResources(const std::string& strExe)
{
    const auto resources = PEResources::Read(strExe);

    if (!resources)
    {
        return std::nullopt;
    }

    auto strings = std::map<unsigned int, std::string>();

    for (const auto& resource : *resources)
    {
        if (resource.nType != PEResources::kTypeString || resource.nID == 0)
        {
            continue;
        }

        const auto& data = resource.data;

        const auto u16 = [&](size_t nAt) -> unsigned int
        {
            if (nAt + 2 > data.size())
            {
                return 0;
            }

            return static_cast<unsigned int>(data[nAt] | data[nAt + 1] << 8);
        };

        auto nAt = size_t(0);

        for (unsigned int nString = 0; nString < 16 && nAt + 2 <= data.size(); nString++)
        {
            const auto nLength = u16(nAt);

            nAt += 2;

            auto text = std::string();

            for (unsigned int n = 0; n < nLength; n++)
            {
                auto nCode = u16(nAt + n * 2);

                if (nCode >= 0xd800 && nCode < 0xdc00 && n + 1 < nLength)
                {
                    nCode = 0x10000 + ((nCode - 0xd800) << 10) + (u16(nAt + (n + 1) * 2) - 0xdc00);
                    n++;
                }

                AppendUTF8(text, nCode);
            }

            nAt += nLength * 2;

            const auto nID = (resource.nID - 1) * 16 + nString;

            if (nLength > 0 && nID < kMaxStringID)
            {
                strings[nID] = text;
            }
        }
    }

    return strings;
}

// Both releases of the game carry the same string table.
bool WriteFromGame(const std::string& strExe, const std::string& strFile)
{
    auto read = ReadStringResources(strExe);

    if (!read)
    {
        return false;
    }

    auto& strings = *read;

    for (const auto& [nID, pszText] : s_aOverrides)
    {
        strings[static_cast<unsigned int>(nID)] = pszText;
    }

    auto file = std::ofstream(strFile, std::ios::binary | std::ios::trunc);

    file << "# HyperBlade's text, read from HYPERX.EXE. Edit the text after each = to change it.\n";
    file << "# \\n is a new line and \\t a tab. Delete this file to read it from the game again.\n";

    for (const auto& [nID, text] : strings)
    {
        char szID[16];

        snprintf(szID, sizeof(szID), "0x%04x", nID);

        file << szID << '=' << Escape(text) << '\n';
    }

    return file.good();
}

} // namespace

bool StringTable::Load(const char* pszDirectory)
{
    const auto strDirectory = std::string(pszDirectory);
    const auto strFile      = Path::Native((strDirectory + kFileName).c_str());

    auto file = std::ifstream(strFile, std::ios::binary);

    if (!file)
    {
        if (!WriteFromGame(Path::Native((strDirectory + "HYPERX.EXE").c_str()), strFile))
        {
            return false;
        }

        file.open(strFile, std::ios::binary);
    }

    s_strings.clear();

    auto line = std::string();

    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        const auto nEquals = line.find('=');

        if (line.empty() || line[0] == '#' || nEquals == std::string::npos)
        {
            continue;
        }

        const auto nID = std::strtoul(line.substr(0, nEquals).c_str(), nullptr, 0);

        s_strings[static_cast<unsigned int>(nID)] = ToGameText(Unescape(std::string_view(line).substr(nEquals + 1)));
    }

    return true;
}

const char* StringTable::Find(unsigned int nID)
{
    const auto it = s_strings.find(nID);

    if (it == s_strings.end())
    {
        return "";
    }

    return it->second.c_str();
}
