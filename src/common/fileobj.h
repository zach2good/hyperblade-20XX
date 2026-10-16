// The model loader's file handle.

#ifndef HYPERBLADE_FILEOBJ_H
#define HYPERBLADE_FILEOBJ_H

// FileObj::Open's modes. Every mode opens in binary.
enum FileMode
{
    FILE_MODE_READ              = 1,
    FILE_MODE_WRITE             = 2,
    FILE_MODE_WRITE_NO_TRUNCATE = 3
};

// FileObj::SeekRelative's origins. Any other value goes to _lseek as it is.
enum FileSeekOrigin
{
    FILE_SEEK_START   = 1,
    FILE_SEEK_CURRENT = 2,
    FILE_SEEK_END     = 3
};

class FileObj
{
public:
    void Close();

    // Answers whether the descriptor closed cleanly.
    int  CloseAndTest();
    char Open(const char* pszName, int nMode);
    int  Read(void* pBuffer, int nCount);
    int  SeekRelative(long nOffset, int nOrigin);
    int  GetNextLine(char** ppLine);

    int Write(const void* pBuf, unsigned int nCount);

    int PrintFormatted(const char* pszFormat, ...);

    int  m_fd;          // 0x000
    char m_szLine[256]; // 0x004
};

#include "common/ownedfile.h"
#include "graphics/mdm1.h"
#include "graphics/modelfile.h"

#endif
