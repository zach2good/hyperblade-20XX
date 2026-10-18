// The animation set loader's free functions, and the script table's size.

#ifndef HYPERBLADE_ANIMSET_H
#define HYPERBLADE_ANIMSET_H
#include "common/compat.h"
#include "common/pathstring.h"

#include "anim/animscriptstate.h"

constexpr auto kAnimsetEntries = 300;

#include "anim/animscripttable.h"

class FileObj;
class AnimClipSet;
class AnimRegistry;

namespace AnimSet
{

int          GetFieldOffset(short nKind);
AnimClipSet* LoadIntoRegistry(FileObj* pFile, AnimRegistry* pRegistry, int nUnused, const char* pszName);

} // namespace AnimSet

#endif
