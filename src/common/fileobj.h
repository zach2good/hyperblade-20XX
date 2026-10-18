// The model loader's file handle, over an SDL stream.

#ifndef HYPERBLADE_FILEOBJ_H
#define HYPERBLADE_FILEOBJ_H

#include "common/handle.h"

// FileObj::Open's modes. Every mode opens in binary.
enum class FileMode
{
    kRead            = 1,
    kWrite           = 2,
    kWriteNoTruncate = 3
};

// FileObj::SeekRelative's origins, one to three where SDL counts from zero.
enum class FileSeekOrigin
{
    kStart   = 1,
    kCurrent = 2,
    kEnd     = 3
};

class FileObj
{
public:
    void Close();

    // Answers whether the file closed cleanly.
    int  CloseAndTest();
    char Open(const char* pszName, FileMode mode);
    int  Read(void* pBuffer, int nCount);
    int  SeekRelative(long nOffset, FileSeekOrigin origin);
    int  GetNextLine(char** ppLine);

    int Write(const void* pBuf, unsigned int nCount);

    int PrintFormatted(const char* pszFormat, ...);

    using Stream = Handle<struct FileObjStreamTag>;

    Stream m_stream;
    char   m_szLine[256];
};

#include "common/ownedfile.h"
#include "graphics/mdm1.h"
#include "graphics/modelfile.h"

#endif
