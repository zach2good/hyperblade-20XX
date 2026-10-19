// Applies every plan fix to the game's plans. With a second folder, writes the fixed plans there.

#include "planvm/planpatch.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

static std::vector<int> ReadPlan(const std::filesystem::path& path)
{
    auto file   = std::ifstream(path, std::ios::binary);
    auto nCount = 0u;

    if (!file.read(reinterpret_cast<char*>(&nCount), sizeof(nCount)))
    {
        return {};
    }

    auto words = std::vector<int>(nCount);

    if (!file.read(reinterpret_cast<char*>(words.data()), static_cast<std::streamsize>(nCount * sizeof(int))))
    {
        return {};
    }

    return words;
}

static void WritePlan(const std::filesystem::path& path, const std::vector<int>& words)
{
    auto       file   = std::ofstream(path, std::ios::binary | std::ios::trunc);
    const auto nCount = static_cast<unsigned int>(words.size());

    file.write(reinterpret_cast<const char*>(&nCount), sizeof(nCount));
    file.write(reinterpret_cast<const char*>(words.data()), static_cast<std::streamsize>(words.size() * sizeof(int)));
}

int main(int nArgs, char** ppszArgs)
{
    if (nArgs != 2 && nArgs != 3)
    {
        std::printf("usage: planpatch_check <Data folder> [output folder]\n");

        return 2;
    }

    const auto plans  = std::filesystem::path(ppszArgs[1]) / "PLANS";
    auto       nFails = 0;

    for (const auto name : PlanPatch::Names())
    {
        const auto strFile = std::string(name) + ".PBN";
        auto       words   = ReadPlan(plans / strFile);
        const auto fixed   = PlanPatch::Fixed(name, words);

        if (!fixed.has_value() || *fixed == words)
        {
            std::printf("not fixed: %s\n", strFile.c_str());
            nFails++;

            continue;
        }

        if (nArgs == 3)
        {
            WritePlan(std::filesystem::path(ppszArgs[2]) / strFile, *fixed);
        }

        // A plan that differs by one word is left alone.
        words[words.size() / 2]++;

        if (PlanPatch::Fixed(name, words).has_value())
        {
            std::printf("fixed a changed plan: %s\n", strFile.c_str());
            nFails++;
        }
    }

    std::printf("%zu plans, %d failed\n", PlanPatch::Names().size(), nFails);

    return nFails == 0 ? 0 : 1;
}
