// Reads the built pak back and checks each named file matches its source, byte for byte.

#include "common/pak.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <vector>

int main(int nArgs, char** ppszArgs)
{
    if (nArgs < 3)
    {
        std::println("usage: pak_check <pak> <res folder> <files...>");
        return 1;
    }

    if (!Pak::Open(ppszArgs[1]))
    {
        std::println("failed: {} does not open", ppszArgs[1]);
        return 1;
    }

    auto nFailed = 0;

    for (auto i = 3; i < nArgs; i++)
    {
        auto       file     = std::ifstream(std::string(ppszArgs[2]) + "/" + ppszArgs[i], std::ios::binary);
        const auto expected = std::vector<unsigned char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        if (expected.empty() || Pak::Read(ppszArgs[i]) != expected)
        {
            std::println("failed: {}", ppszArgs[i]);
            nFailed++;
        }
    }

    if (!Pak::Read("missing.bin").empty())
    {
        std::println("failed: a missing file reads as empty");
        nFailed++;
    }

    return nFailed;
}
