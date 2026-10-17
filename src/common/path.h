#ifndef HYPERBLADE_PATH_H
#define HYPERBLADE_PATH_H

#include <string>

namespace Path
{

// A game path as the host can open it. The game names its files the Windows way, so backslashes
// become slashes and, where the exact path is missing, each part takes the name on disk that
// matches it without regard to case.
std::string Native(const char* pszPath);

} // namespace Path

#endif
