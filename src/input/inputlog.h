// The input the game received, for the debug menu: the last few presses and releases, and the
// keys held now. Keys are the virtual-key codes the game reads.

#ifndef HYPERBLADE_INPUTLOG_H
#define HYPERBLADE_INPUTLOG_H

#include <cstdint>

namespace InputLog
{

struct Entry
{
    std::uint64_t nTicksMs;
    const char*   pszSource; // "Keyboard", "Mouse", or the pad control's name.
    const char*   pszAction; // The pad's action, or null.
    unsigned int  nKey;
    bool          bDown;
};

// pszSource and pszAction must live for the whole run, as string literals do.
void Add(const char* pszSource, unsigned int nKey, bool bDown, const char* pszAction);

// Newest first; nIndex runs up to Count() - 1.
int          Count();
const Entry& Recent(int nIndex);

bool IsHeld(unsigned int nKey);

} // namespace InputLog

#endif
