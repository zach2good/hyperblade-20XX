#include "stringtable.h"
#include "app/stringids.h"
#include <cstdio>
#include <array>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace
{

// The game's ids stop well short of this.
constexpr auto MAX_STRING_ID = 0x1000u;

// The original pointed players at a joystick control panel and a DOS set-up tool.
constexpr auto s_aOverrides = std::array{
    std::pair{ STRING_JOYSTICK_CONTROL_PANEL, "Plug in an Xbox, PlayStation or Switch controller at any time.\n\n"
                                              "Skate with the left stick or d-pad. A attacks, LB is a speed burst, X shoots and Y passes. "
                                              "Start pauses.\n\nIn the menus, A selects and B goes back. " },
    std::pair{ STRING_GAME_PAD_HELP, "The [Gamepad] section of booie.ini picks the action for each button and trigger." },
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
        const auto nLead = (unsigned char)utf8[i];

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

        auto nCode = (unsigned int)(nLead & (0x7f >> nLength));

        for (int n = 1; n < nLength && i + n < utf8.size(); n++)
        {
            nCode = nCode << 6 | ((unsigned char)utf8[i + n] & 0x3f);
        }

        if (nLength == 1)
        {
            nCode = nLead;
        }

        text += [&]
        {
            if (nCode < 0x100)
            {
                return (char)nCode;
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
        out += (char)nCode;
    }
    else if (nCode < 0x800)
    {
        out += (char)(0xc0 | nCode >> 6);
        out += (char)(0x80 | (nCode & 0x3f));
    }
    else if (nCode < 0x10000)
    {
        out += (char)(0xe0 | nCode >> 12);
        out += (char)(0x80 | (nCode >> 6 & 0x3f));
        out += (char)(0x80 | (nCode & 0x3f));
    }
    else
    {
        out += (char)(0xf0 | nCode >> 18);
        out += (char)(0x80 | (nCode >> 12 & 0x3f));
        out += (char)(0x80 | (nCode >> 6 & 0x3f));
        out += (char)(0x80 | (nCode & 0x3f));
    }
}

// Little-endian reads that mark the image bad instead of reading past its end.
struct Image
{
    std::vector<unsigned char> bytes;
    bool                       bValid = true;

    unsigned int U16(size_t nAt)
    {
        if (nAt + 2 > bytes.size())
        {
            bValid = false;

            return 0;
        }

        return (unsigned int)(bytes[nAt] | bytes[nAt + 1] << 8);
    }

    unsigned int U32(size_t nAt)
    {
        return U16(nAt) | U16(nAt + 2) << 16;
    }
};

constexpr auto RT_STRING_ID     = 6u;
constexpr auto SUBDIRECTORY     = 0x80000000u;
constexpr auto MAX_RESOURCE_DIR = 4096u;

// The program's string resources, read from its file without loading it. Under RT_STRING, block n
// holds ids (n - 1) * 16 onward as sixteen counted UTF-16 strings. The first language is taken.
std::optional<std::map<unsigned int, std::string> > ReadStringResources(const std::string& strExe)
{
    auto file = std::ifstream(strExe, std::ios::binary);

    if (!file)
    {
        return std::nullopt;
    }

    auto image = Image{ std::vector<unsigned char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()) };

    const auto nPE = (size_t)image.U32(0x3c);

    if (image.U32(nPE) != 0x00004550)
    {
        return std::nullopt;
    }

    const auto nCOFF     = nPE + 4;
    const auto nSections = image.U16(nCOFF + 2);
    const auto nOptional = nCOFF + 20;

    const auto nDirectories = [&]
    {
        // PE32+ widens the fields before the data directories.
        if (image.U16(nOptional) == 0x20b)
        {
            return nOptional + 112;
        }

        return nOptional + 96;
    }();

    const auto nSectionTable = nOptional + image.U16(nCOFF + 16);

    const auto toOffset = [&](unsigned int nRVA) -> size_t
    {
        for (unsigned int nSection = 0; nSection < nSections; nSection++)
        {
            const auto nAt      = nSectionTable + nSection * 40;
            const auto nAddress = image.U32(nAt + 12);
            const auto nSize    = (std::max)(image.U32(nAt + 8), image.U32(nAt + 16));

            if (nRVA >= nAddress && nRVA - nAddress < nSize)
            {
                return image.U32(nAt + 20) + (size_t)(nRVA - nAddress);
            }
        }

        image.bValid = false;

        return 0;
    };

    const auto nRoot = toOffset(image.U32(nDirectories + 2 * 8));

    // Each directory entry is an id and an offset from the resource root.
    const auto entries = [&](size_t nDirectory)
    {
        auto found = std::vector<std::pair<unsigned int, unsigned int> >();

        const auto nCount = (std::min)(image.U16(nDirectory + 12) + image.U16(nDirectory + 14), MAX_RESOURCE_DIR);

        for (unsigned int n = 0; n < nCount && image.bValid; n++)
        {
            found.emplace_back(image.U32(nDirectory + 16 + n * 8), image.U32(nDirectory + 20 + n * 8));
        }

        return found;
    };

    auto strings = std::map<unsigned int, std::string>();

    for (const auto& [nType, nTypeAt] : entries(nRoot))
    {
        if (nType != RT_STRING_ID || (nTypeAt & SUBDIRECTORY) == 0)
        {
            continue;
        }

        for (const auto& [nBlock, nBlockAt] : entries(nRoot + (nTypeAt & ~SUBDIRECTORY)))
        {
            if ((nBlockAt & SUBDIRECTORY) == 0)
            {
                continue;
            }

            const auto languages = entries(nRoot + (nBlockAt & ~SUBDIRECTORY));

            if (languages.empty() || (languages[0].second & SUBDIRECTORY) != 0)
            {
                continue;
            }

            const auto nEntry = nRoot + languages[0].second;

            auto nAt = toOffset(image.U32(nEntry));

            for (unsigned int nString = 0; nString < 16 && image.bValid; nString++)
            {
                const auto nLength = image.U16(nAt);

                nAt += 2;

                auto text = std::string();

                for (unsigned int n = 0; n < nLength && image.bValid; n++)
                {
                    auto nCode = image.U16(nAt + n * 2);

                    if (nCode >= 0xd800 && nCode < 0xdc00 && n + 1 < nLength)
                    {
                        nCode = 0x10000 + ((nCode - 0xd800) << 10) + (image.U16(nAt + (n + 1) * 2) - 0xdc00);
                        n++;
                    }

                    AppendUTF8(text, nCode);
                }

                nAt += nLength * 2;

                const auto nID = (nBlock - 1) * 16 + nString;

                if (nLength > 0 && nID < MAX_STRING_ID)
                {
                    strings[nID] = text;
                }
            }
        }
    }

    if (!image.bValid)
    {
        return std::nullopt;
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
        strings[(unsigned int)nID] = pszText;
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
    const auto strFile      = strDirectory + FILE_NAME;

    auto file = std::ifstream(strFile, std::ios::binary);

    if (!file)
    {
        if (!WriteFromGame(strDirectory + "HYPERX.EXE", strFile))
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

        s_strings[(unsigned int)nID] = ToGameText(Unescape(std::string_view(line).substr(nEquals + 1)));
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
