// Reads the built pak back and checks each named file matches its source, byte for byte.

#include "common/pak.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main(int nArgs, char** ppszArgs)
{
    if (nArgs < 3)
    {
        std::printf("usage: pak_check <pak> <res folder> <files...>\n");
        return 1;
    }

    if (!Pak::Open(ppszArgs[1]))
    {
        std::printf("failed: %s does not open\n", ppszArgs[1]);
        return 1;
    }

    auto nFailed = 0;

    for (auto i = 3; i < nArgs; i++)
    {
        auto       file     = std::ifstream(std::string(ppszArgs[2]) + "/" + ppszArgs[i], std::ios::binary);
        const auto expected = std::vector<unsigned char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        if (expected.empty() || Pak::Read(ppszArgs[i]) != expected)
        {
            std::printf("failed: %s\n", ppszArgs[i]);
            nFailed++;
        }
    }

    if (!Pak::Read("missing.bin").empty())
    {
        std::printf("failed: a missing file reads as empty\n");
        nFailed++;
    }

    return nFailed;
}
