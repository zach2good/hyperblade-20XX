// Round-trips booie.ini reads and writes through a scratch file.

#include "common/inifile.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

static int s_nFailed = 0;

static void Expect(bool bOK, const char* pszWhat)
{
    if (!bOK)
    {
        std::printf("failed: %s\n", pszWhat);
        s_nFailed++;
    }
}

int main()
{
    const auto path    = std::filesystem::temp_directory_path() / "hyperx_ini_check.ini";
    const auto strPath = path.string();
    const auto pszPath = strPath.c_str();

    {
        auto file = std::ofstream(path, std::ios::binary | std::ios::trunc);
        file << "; comment\r\n[Options]\r\nInstallLevel = 2\r\nNegative=-4\r\n\r\n[Keys]\r\nForward=38\r\n";
    }

    Expect(IniFile::ReadInt(pszPath, "options", "installlevel", 7) == 2, "case and spaces ignored");
    Expect(IniFile::ReadInt(pszPath, "Options", "Negative", 7) == 0, "negative reads as zero");
    Expect(IniFile::ReadInt(pszPath, "Options", "Missing", 7) == 7, "missing key gives default");
    Expect(IniFile::ReadInt(pszPath, "Nowhere", "Forward", 9) == 9, "missing section gives default");
    Expect(IniFile::ReadInt(pszPath, "Options", "Forward", 9) == 9, "keys stay in their section");
    Expect(IniFile::ReadString(pszPath, "options", "INSTALLLEVEL", "x") == "2", "string read trimmed");
    Expect(IniFile::ReadString(pszPath, "Options", "Missing", "x") == "x", "missing string gives default");

    Expect(IniFile::WriteString(pszPath, "Keys", "Forward", "87"), "write existing key");
    Expect(IniFile::WriteString(pszPath, "Options", "SfxVolume", "5"), "write new key");
    Expect(IniFile::WriteString(pszPath, "Gamepad", "Attack", "1"), "write new section");

    Expect(IniFile::ReadInt(pszPath, "Keys", "Forward", 0) == 87, "existing key replaced");
    Expect(IniFile::ReadInt(pszPath, "Options", "SfxVolume", 0) == 5, "new key added to its section");
    Expect(IniFile::ReadInt(pszPath, "Gamepad", "Attack", 0) == 1, "new section appended");
    Expect(IniFile::ReadInt(pszPath, "Options", "InstallLevel", 0) == 2, "other keys kept");

    auto text = std::stringstream();
    text << std::ifstream(path, std::ios::binary).rdbuf();
    Expect(text.str().rfind("; comment\r\n[Options]\r\nInstallLevel = 2\r\nNegative=-4\r\nSfxVolume=5\r\n", 0) == 0, "lines kept in place");

    std::filesystem::remove(path);

    std::printf("%d failed\n", s_nFailed);

    if (s_nFailed != 0)
    {
        return 1;
    }

    return 0;
}
