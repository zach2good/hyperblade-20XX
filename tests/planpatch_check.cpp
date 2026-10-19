// Applies every plan fix in res/plans to the game's plans. With an output folder, also writes each
// fixed plan and every plan's decompiled source there, to write new fixes against.

#include "planvm/planpatch.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
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

static std::string ReadText(const std::filesystem::path& path)
{
    auto stream = std::ostringstream();

    stream << std::ifstream(path, std::ios::binary).rdbuf();

    return stream.str();
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
    if (nArgs != 3 && nArgs != 4)
    {
        std::printf("usage: planpatch_check <Data folder> <res/plans folder> [output folder]\n");

        return 2;
    }

    auto nFails = 0;

    // A hunk whose old lines are not there is refused.
    if (PlanPatch::ApplyDiff("a\nb\nc\n", "@@ -2,1 +2,1 @@\n-b\n+B\n") != "a\nB\nc\n" || PlanPatch::ApplyDiff("a\nb\nc\n", "@@ -2,1 +2,1 @@\n-x\n+B\n").has_value())
    {
        std::printf("ApplyDiff is wrong\n");
        nFails++;
    }

    const auto data     = std::filesystem::path(ppszArgs[1]);
    const auto compiler = PlanPatch::Compiler::FromDataDir(data.string());

    if (!compiler.has_value())
    {
        std::printf("no compiler symbols in %s\n", ppszArgs[1]);

        return 1;
    }

    auto nPlans = 0;

    for (const auto& entry : std::filesystem::directory_iterator(ppszArgs[2]))
    {
        if (entry.path().extension() != ".diff")
        {
            continue;
        }

        nPlans++;

        const auto strName = entry.path().stem().string();
        const auto words   = ReadPlan(data / "PLANS" / (strName + ".PBN"));
        const auto fixed   = compiler->Fixed(words, ReadText(entry.path()));

        if (!fixed.has_value() || *fixed == words)
        {
            std::printf("not fixed: %s\n", strName.c_str());
            nFails++;

            continue;
        }

        if (nArgs == 4)
        {
            WritePlan(std::filesystem::path(ppszArgs[3]) / (strName + ".PBN"), *fixed);
        }
    }

    if (nArgs == 4)
    {
        for (const auto& entry : std::filesystem::directory_iterator(data / "PLANS"))
        {
            const auto source = compiler->Decompile(ReadPlan(entry.path()));

            if (entry.path().extension() == ".PBN" && source.has_value())
            {
                std::ofstream(std::filesystem::path(ppszArgs[3]) / (entry.path().stem().string() + ".txt"), std::ios::binary) << *source;
            }
        }
    }

    std::printf("%d plans, %d failed\n", nPlans, nFails);

    if (nFails != 0)
    {
        return 1;
    }

    return 0;
}
