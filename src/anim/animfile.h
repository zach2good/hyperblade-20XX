#ifndef HYPERBLADE_ANIMFILE_H
#define HYPERBLADE_ANIMFILE_H

#include "common/compat.h"

class FileObj;
class AnimClipSet;
class AnimRegistry;

typedef AnimClipSet*(__cdecl* AnimLoadFn)(FileObj* pFile, AnimRegistry* pRegistry, int nFlags, const char* pszName);

namespace AnimFile
{

short                VersionCompare(const struct FileVersion* pLeft, const struct FileVersion* pRight);
int __cdecl          CheckVersion(const char* pszName, const struct FileVersion* pExpected, const struct FileVersion* pFound);
AnimClipSet* __cdecl Open(const char* pszName, AnimRegistry* pRegistry, int nFlags, AnimLoadFn pfnLoad);

} // namespace AnimFile

#endif
