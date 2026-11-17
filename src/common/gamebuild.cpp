#include "common/gamebuild.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>

namespace
{

constexpr auto kOffsetOfPEHeader = 0x3c;
constexpr auto kOffsetOfLinkTime = 8;

struct KnownBuild
{
    std::uint32_t nLinkTime;
    GameBuild     build;
};

constexpr auto kKnownBuilds = std::array<KnownBuild, 3>{ {
    { 0x321ac189, GameBuild::Demo },
    { 0x326bcbde, GameBuild::V1000 },
    { 0x3957d6df, GameBuild::V1001 },
} };

GameBuild s_current = GameBuild::Unknown;

bool ReadUint32(std::ifstream& file, std::streamoff offset, std::uint32_t& nValue)
{
    auto bytes = std::array<char, 4>{};

    file.seekg(offset);

    if (!file.read(bytes.data(), bytes.size()))
    {
        return false;
    }

    std::memcpy(&nValue, bytes.data(), bytes.size());

    return true;
}

} // namespace

GameBuild GameBuilds::Detect(const std::string& strExe)
{
    auto file = std::ifstream(strExe, std::ios::binary);

    auto nPEHeader = std::uint32_t{};
    auto nLinkTime = std::uint32_t{};

    if (!ReadUint32(file, kOffsetOfPEHeader, nPEHeader) || !ReadUint32(file, nPEHeader + kOffsetOfLinkTime, nLinkTime))
    {
        return GameBuild::Unknown;
    }

    for (const auto& known : kKnownBuilds)
    {
        if (known.nLinkTime == nLinkTime)
        {
            return known.build;
        }
    }

    return GameBuild::Unknown;
}

GameBuild GameBuilds::Current()
{
    return s_current;
}

void GameBuilds::SetCurrent(GameBuild build)
{
    s_current = build;
}

const char* GameBuilds::Name(GameBuild build)
{
    switch (build)
    {
        case GameBuild::Demo:
            return "the August 1996 demo";

        case GameBuild::V1000:
            return "v1.0.0.0";

        case GameBuild::V1001:
            return "v1.0.0.1";

        case GameBuild::Unknown:
            break;
    }

    return "an unknown build";
}
