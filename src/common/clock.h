// The game's clock: milliseconds since start-up, as std::chrono durations.

#ifndef HYPERBLADE_CLOCK_H
#define HYPERBLADE_CLOCK_H

#include <chrono>
#include <cstdint>

namespace Clock
{

// Milliseconds in 32 bits, which wrap after 49 days as the original's timeGetTime did.
using Milliseconds = std::chrono::duration<std::uint32_t, std::milli>;

// Milliseconds with a fraction, for sums and averages.
using FractionalMilliseconds = std::chrono::duration<double, std::milli>;

// Both sit in fields that were a uint32 and a double.
static_assert(sizeof(Milliseconds) == 4 && sizeof(FractionalMilliseconds) == 8);

inline const auto kStart = std::chrono::steady_clock::now();

// A batch simulation steps the clock a tick at a time, so plan timers and the period clock keep
// pace with physics that runs as fast as the CPU allows.
inline bool         g_bSimulated = false;
inline Milliseconds g_simulatedNow{};

inline Milliseconds Now()
{
    if (g_bSimulated)
    {
        return g_simulatedNow;
    }

    return std::chrono::duration_cast<Milliseconds>(std::chrono::steady_clock::now() - kStart);
}

} // namespace Clock

#endif
