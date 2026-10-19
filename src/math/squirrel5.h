#ifndef HYPERBLADE_SQUIRREL5_H
#define HYPERBLADE_SQUIRREL5_H

#include <cstdint>

namespace Math
{

// Largest value Rand() returns.
constexpr int kRandMax = 0x7fffffff;

// Squirrel Eiserloh's SquirrelNoise5, a stateless hash of a position and a seed.
constexpr uint32_t Squirrel5(uint32_t nPosition, uint32_t nSeed)
{
    constexpr uint32_t kBitNoise1 = 0xd2a80a23;
    constexpr uint32_t kBitNoise2 = 0xa884f197;
    constexpr uint32_t kBitNoise3 = 0x6c736f4b;
    constexpr uint32_t kBitNoise4 = 0xb79f3abb;
    constexpr uint32_t kBitNoise5 = 0x1b56c4f5;

    auto nMangled = nPosition * kBitNoise1;
    nMangled += nSeed;
    nMangled ^= nMangled >> 9;
    nMangled += kBitNoise2;
    nMangled ^= nMangled >> 11;
    nMangled *= kBitNoise3;
    nMangled ^= nMangled >> 13;
    nMangled += kBitNoise4;
    nMangled ^= nMangled << 15;
    nMangled *= kBitNoise5;
    nMangled ^= nMangled >> 17;

    return nMangled;
}

inline uint32_t g_nRandSeed     = 0;
inline uint32_t g_nRandPosition = 0;

// Like srand(), restarts the sequence.
inline void SeedRand(uint32_t nSeed)
{
    g_nRandSeed     = nSeed;
    g_nRandPosition = 0;
}

// A uniform integer in [0, kRandMax].
inline int Rand()
{
    return static_cast<int>(Squirrel5(g_nRandPosition++, g_nRandSeed) >> 1);
}

} // namespace Math

#endif
