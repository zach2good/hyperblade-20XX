#ifndef HYPERBLADE_MODELFILE_H
#define HYPERBLADE_MODELFILE_H

#include "common/compat.h"

class FileObj;

// The version a model or animation file carries; VersionCompare reads every field.
struct FileVersion
{
    short nMajor;    // 0x00
    short nMinor;    // 0x02
    short nRevision; // 0x04
    short nUnused;   // 0x06
    int   nBuild;    // 0x08
};

// The 0x30 bytes every model and animation file opens with.
struct FileHeader
{
    int         nTag;         // 0x00
    char        szName[0x20]; // 0x04
    FileVersion version;      // 0x24
};

static_assert(sizeof(FileHeader) == 0x30);

namespace ModelFile
{

char Open(FileObj* pFile, const char* pszModelName);
int  ReadHeader(FileObj* pFile, int* pnTag, char* pszName, FileVersion* pVersion);

} // namespace ModelFile

#endif
