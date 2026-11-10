// Reads lines however the file ends them. The file is opened in binary, so Windows sees the CRLFs
// Linux does.

#include "common/parseutil.h"
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

static int s_nFailed = 0;

static void Expect(bool bOK, const std::string& strWhat)
{
    if (!bOK)
    {
        std::printf("failed: %s\n", strWhat.c_str());
        s_nFailed++;
    }
}

// Every line ReadLine returns until the end, as text.
static std::vector<std::string> ReadAll(const std::filesystem::path& file, std::string_view bytes)
{
    std::ofstream(file, std::ios::binary) << bytes;

    auto lines = std::vector<std::string>();

    auto* const pFile = std::fopen(file.string().c_str(), "rb");

    if (pFile == nullptr)
    {
        return lines;
    }

    auto buffer = std::array<char, 256>{};

    // Bounded, so a broken reader can never loop forever.
    for (int n = 0; n < 16; n++)
    {
        const auto nLength = ParseUtil::ReadLine(pFile, buffer.data());

        if (nLength < 0)
        {
            break;
        }

        Expect(nLength == static_cast<int>(std::string_view(buffer.data()).size()), "the length is the line's");

        lines.emplace_back(buffer.data());
    }

    std::fclose(pFile);

    return lines;
}

int main()
{
    const auto file = std::filesystem::temp_directory_path() / "hyperx_parseutil_check.txt";

    struct Case
    {
        std::string_view         bytes;
        std::vector<std::string> lines;
        const char*              pszWhat;
    };

    const Case cases[] = {
        { "a\r\nb\r\n", { "a", "b" }, "CRLF lines" },
        { "a\nb\n", { "a", "b" }, "LF lines" },
        { "a\rb", { "a", "b" }, "a lone CR ends a line" },
        { "a\r\nb", { "a", "b" }, "a last line without a newline" },
        { "a\r\n\r\nb", { "a", "", "b" }, "an empty CRLF line between" },
    };

    for (const auto& test : cases)
    {
        const auto lines = ReadAll(file, test.bytes);

        Expect(lines == test.lines, test.pszWhat);
    }

    std::filesystem::remove(file);

    return s_nFailed;
}
