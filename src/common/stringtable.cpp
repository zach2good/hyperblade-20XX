#include "stringtable.h"
#include "app/stringids.h"
#include <windows.h>
#include <cstdio>
#include <array>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

namespace
{

// The game's ids stop well short of this.
constexpr auto MAX_STRING_ID = 0x1000u;

// The original pointed players at a joystick control panel and a DOS set-up tool.
constexpr auto s_aOverrides = std::array{
    std::pair{ STRING_JOYSTICK_CONTROL_PANEL, "Plug in an Xbox, PlayStation or Switch controller at any time.\n\n"
                                              "Skate with the left stick or d-pad. A attacks, LB is a speed burst, X shoots and Y passes. "
                                              "Start pauses.\n\nIn the menus, A selects and B goes back. " },
    std::pair{ STRING_GAME_PAD_HELP, "The [Gamepad] section of booie.ini picks the action for each face button." },
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

// The file is UTF-8 and the game draws with GDI's ANSI calls, so the text arrives in the ANSI code page.
std::string ToGameText(const std::string& utf8)
{
    const auto nWide = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);

    auto wide = std::wstring((size_t)nWide, L'\0');

    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), wide.data(), nWide);

    const auto nAnsi = WideCharToMultiByte(CP_ACP, 0, wide.data(), nWide, nullptr, 0, nullptr, nullptr);

    auto ansi = std::string((size_t)nAnsi, '\0');

    WideCharToMultiByte(CP_ACP, 0, wide.data(), nWide, ansi.data(), nAnsi, nullptr, nullptr);

    return ansi;
}

std::string ToUTF8(const wchar_t* pszWide, int nLength)
{
    const auto nUTF8 = WideCharToMultiByte(CP_UTF8, 0, pszWide, nLength, nullptr, 0, nullptr, nullptr);

    auto utf8 = std::string((size_t)nUTF8, '\0');

    WideCharToMultiByte(CP_UTF8, 0, pszWide, nLength, utf8.data(), nUTF8, nullptr, nullptr);

    return utf8;
}

// Both releases of the game carry the same string table.
bool WriteFromGame(const std::string& strExe, const std::string& strFile)
{
    const auto hModule = LoadLibraryExA(strExe.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);

    if (hModule == nullptr)
    {
        return false;
    }

    auto strings = std::map<unsigned int, std::string>();

    for (auto nID = 0u; nID < MAX_STRING_ID; nID++)
    {
        const wchar_t* pszText = nullptr;

        const auto nLength = LoadStringW(hModule, nID, (LPWSTR)&pszText, 0);

        if (nLength > 0)
        {
            strings[nID] = ToUTF8(pszText, nLength);
        }
    }

    FreeLibrary(hModule);

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
