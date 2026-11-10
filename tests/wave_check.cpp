// Parses every wave in the game's Sounds folder and reads its samples back.

#include "sound/wave.h"
#include <cstdio>
#include <filesystem>
#include <print>
#include <vector>

int main(int nArgs, char** ppszArgs)
{
    if (nArgs != 2)
    {
        std::println("usage: wave_check <Data folder>");

        return 2;
    }

    auto nChecked = 0;
    auto nFailed  = 0;

    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(ppszArgs[1]) / "SOUNDS"))
    {
        const auto strPath = entry.path().string();

        if (entry.path().extension() != ".WAV" && entry.path().extension() != ".wav")
        {
            continue;
        }

        nChecked++;

        auto info = Wave::Info{};

        if (Wave::ReadInfo(strPath.c_str(), info) != Wave::Result::OK || info.nChannels == 0 || info.nSamplesPerSec == 0)
        {
            std::println("bad header: {}", strPath);
            nFailed++;

            continue;
        }

        auto samples = std::vector<unsigned char>(info.nDataBytes);

        if (!Wave::ReadSamples(strPath.c_str(), info.nDataOffset, info.nDataBytes, samples.data()))
        {
            std::println("short data: {}", strPath);
            nFailed++;
        }
    }

    std::println("{} waves, {} failed", nChecked, nFailed);

    if (nChecked == 0 || nFailed != 0)
    {
        return 1;
    }

    return 0;
}
