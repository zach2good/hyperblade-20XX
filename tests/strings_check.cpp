// Reads a strings file back, escapes and comments included.

#include "common/stringtable.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

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
    const auto directory = std::filesystem::temp_directory_path() / "hyperx_strings_check";

    std::filesystem::create_directories(directory);

    {
        auto file = std::ofstream(directory / StringTable::FILE_NAME, std::ios::binary | std::ios::trunc);
        file << "# A comment\r\n0x0043=HyperBlade\r\n0x00a6=Line one\\nLine two\\tTabbed\r\n0x0007=Back\\\\slash\r\n\r\nnot a string\r\n";
    }

    const auto strDirectory = directory.string() + "\\";

    Expect(StringTable::Load(strDirectory.c_str()), "file loads");
    Expect(std::strcmp(StringTable::Find(0x43), "HyperBlade") == 0, "plain text");
    Expect(std::strcmp(StringTable::Find(0xa6), "Line one\nLine two\tTabbed") == 0, "escapes");
    Expect(std::strcmp(StringTable::Find(7), "Back\\slash") == 0, "backslash");
    Expect(std::strcmp(StringTable::Find(0x99), "") == 0, "unknown id is empty");

    std::filesystem::remove_all(directory);

    Expect(!StringTable::Load(strDirectory.c_str()), "no file and no HYPERX.EXE fails");

    std::printf("%d failed\n", s_nFailed);

    if (s_nFailed != 0)
    {
        return 1;
    }

    return 0;
}
