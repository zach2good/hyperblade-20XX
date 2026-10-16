#include "input/inputlog.h"
#include <SDL3/SDL.h>
#include <array>

namespace
{

constexpr auto LOG_SIZE = 32;

std::array<InputLog::Entry, LOG_SIZE> s_aEntries = {};
std::array<bool, 256>                 s_abHeld   = {};
int                                   s_nNext    = 0;
int                                   s_nCount   = 0;

} // namespace

void InputLog::Add(const char* pszSource, unsigned int nKey, bool bDown, const char* pszAction)
{
    s_aEntries[(size_t)s_nNext] = Entry{ SDL_GetTicks(), pszSource, pszAction, nKey, bDown };

    s_nNext  = (s_nNext + 1) % LOG_SIZE;
    s_nCount = (std::min)(s_nCount + 1, LOG_SIZE);

    if (nKey < s_abHeld.size())
    {
        s_abHeld[nKey] = bDown;
    }
}

int InputLog::Count()
{
    return s_nCount;
}

const InputLog::Entry& InputLog::Recent(int nIndex)
{
    return s_aEntries[(size_t)((s_nNext - 1 - nIndex + LOG_SIZE) % LOG_SIZE)];
}

bool InputLog::IsHeld(unsigned int nKey)
{
    return nKey < s_abHeld.size() && s_abHeld[nKey];
}
