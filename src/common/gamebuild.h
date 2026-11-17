// Which HYPERX.EXE the install holds: the August 1996 demo, the October 1996 retail release or the
// June 2000 patch. Each has its own link time, so the PE header tells them apart.

#ifndef HYPERBLADE_GAMEBUILD_H
#define HYPERBLADE_GAMEBUILD_H

#include <string>

enum class GameBuild
{
    Unknown,
    Demo,
    V1000,
    V1001,
};

namespace GameBuilds
{

// Unknown for a missing file, a file that isn't a program, or a changed HYPERX.EXE.
GameBuild Detect(const std::string& strExe);

// The build Detect found at start-up.
GameBuild Current();
void      SetCurrent(GameBuild build);

const char* Name(GameBuild build);

} // namespace GameBuilds

#endif
