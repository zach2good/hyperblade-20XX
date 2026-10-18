// Reads a strings file back, escapes and comments included, and writes one from a program file.

#include "common/stringtable.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

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

    // A program file holding one string block, as HYPERX.EXE carries its text: block 5 holds ids 0x40
    // to 0x4f, and this one sets 0x43 and 0x44.
    {
        auto image = std::vector<unsigned char>(0x400);

        const auto put16 = [&](size_t nAt, unsigned int nValue)
        {
            image[nAt]     = static_cast<unsigned char>(nValue);
            image[nAt + 1] = static_cast<unsigned char>(nValue >> 8);
        };

        const auto put32 = [&](size_t nAt, unsigned int nValue)
        {
            put16(nAt, nValue & 0xffff);
            put16(nAt + 2, nValue >> 16);
        };

        put32(0x3c, 0x40);
        put32(0x40, 0x00004550);
        put16(0x44, 0x14c);
        put16(0x46, 1);
        put16(0x54, 0xe0);
        put16(0x58, 0x10b);
        put32(0xc8, 0x1000);
        put32(0xcc, 0x100);

        put32(0x140, 0x200);
        put32(0x144, 0x1000);
        put32(0x148, 0x200);
        put32(0x14c, 0x200);

        put16(0x20e, 1);
        put32(0x210, 6);
        put32(0x214, 0x80000018);
        put16(0x226, 1);
        put32(0x228, 5);
        put32(0x22c, 0x80000030);
        put16(0x23e, 1);
        put32(0x240, 0x409);
        put32(0x244, 0x48);
        put32(0x248, 0x1060);
        put32(0x24c, 0x40);

        put16(0x266, 2);
        put16(0x268, 'H');
        put16(0x26a, 'i');
        put16(0x26c, 1);
        put16(0x26e, 0xe9);

        std::filesystem::create_directories(directory);

        auto file = std::ofstream(directory / "HYPERX.EXE", std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
    }

    Expect(StringTable::Load(strDirectory.c_str()), "strings read from the program");
    Expect(std::strcmp(StringTable::Find(0x43), "Hi") == 0, "string from its block");
    Expect(std::strcmp(StringTable::Find(0x44), "\xe9") == 0, "accent comes back as Latin-1");
    Expect(std::filesystem::exists(directory / StringTable::FILE_NAME), "strings file written");

    std::filesystem::remove_all(directory);

    std::printf("%d failed\n", s_nFailed);

    if (s_nFailed != 0)
    {
        return 1;
    }

    return 0;
}
