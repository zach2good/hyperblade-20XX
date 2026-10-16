#include "compat.h"
#include "fileobj.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "app/gameapp.h"
#include <stdarg.h>
#include <stdio.h>

#define TEXT_MAX 0x100

// The original closed a failed open's -1, and callers close twice; the modern CRT aborts on both.
// FUNCTION: HYPERX 0x00440460
void FileObj::Close()
{
    if (m_fd > 0)
    {
        _close(m_fd);
        m_fd = -1;
    }
}

// A mode outside the three leaves the flags uninitialised, as in the original.

// FUNCTION: HYPERX 0x00440470
char FileObj::Open(const char* pszPath, int nMode)
{
    char szResolved[MAX_PATH];
    int  nOflag;
    int  nPmode;

    switch (nMode)
    {
        case FILE_MODE_WRITE_NO_TRUNCATE:
            nOflag = _O_BINARY | _O_CREAT | _O_WRONLY;
            nPmode = _S_IWRITE;
            break;

        case FILE_MODE_WRITE:
            nOflag = _O_BINARY | _O_TRUNC | _O_CREAT | _O_RDWR;
            nPmode = _S_IWRITE;
            break;

        case FILE_MODE_READ:
            nOflag = _O_BINARY | _O_RDONLY;
            nPmode = _S_IREAD;
            break;
    }

    m_fd = _open(pszPath, nOflag, nPmode);

    if (m_fd == -1)
    {
        const char* pszSource;

        char* pszDest = szResolved;
        pszSource     = pszPath;

        while (*pszSource != 0)
        {
            *pszDest++ = *pszSource++;
        }

        *pszDest = 0;

        GameApp::ResolveCDRomPath(szResolved);

        m_fd = _open(szResolved, nOflag, nPmode);

        if (m_fd == -1)
        {
            return 0;
        }
    }

    return 1;
}

// The loader's origins are one to three where the C library wants zero to two.
// FUNCTION: HYPERX 0x00440550
int FileObj::SeekRelative(long nOffset, int nOrigin)
{
    switch (nOrigin)
    {
        case FILE_SEEK_START:
            nOrigin = SEEK_SET;
            break;

        case FILE_SEEK_CURRENT:
            nOrigin = SEEK_CUR;
            break;

        case FILE_SEEK_END:
            nOrigin = SEEK_END;
            break;
    }

    return _lseek(m_fd, nOffset, nOrigin);
}

// FUNCTION: HYPERX 0x004405b0
int FileObj::Write(const void* pBuf, unsigned int nCount)
{
    return _write(m_fd, pBuf, nCount);
}

// Each line feed gains a carriage return, so a line of line feeds overruns the second buffer.
// FUNCTION: HYPERX 0x004405d0
int FileObj::PrintFormatted(const char* pszFormat, ...)
{
    char    szText[TEXT_MAX];
    char    szOut[TEXT_MAX];
    va_list argList;

    va_start(argList, pszFormat);

    vsprintf(szText, pszFormat, argList);

    va_end(argList);

    char  ch    = szText[0];
    char* pSrc  = szText;
    char* pDst  = szOut;
    int   nLeft = TEXT_MAX;

    while (ch != 0 && nLeft != 0)
    {
        if (*pSrc == '\n')
        {
            *pDst = '\r';
            pDst++;
            *pDst = *pSrc;
        }
        else
        {
            *pDst = *pSrc;
        }

        ch = pSrc[1];
        pDst++;
        pSrc++;
        nLeft--;
    }

    *pDst = 0;

    return _write(m_fd, szOut, (unsigned int)strlen(szOut));
}

// FUNCTION: HYPERX 0x00440670
int FileObj::Read(void* pBuffer, int nCount)
{
    return _read(m_fd, pBuffer, nCount);
}

// FUNCTION: HYPERX 0x00440690
int FileObj::CloseAndTest()
{
    if (m_fd <= 0)
    {
        return 0;
    }

    const int nResult = _close(m_fd);
    m_fd              = -1;

    return nResult == 0;
}

// FUNCTION: HYPERX 0x004406b0
int FileObj::GetNextLine(char** ppLine)
{
    char chSkipped;

    char* const pBuffer = m_szLine;

    memset(pBuffer, 0, sizeof(m_szLine));

    int   nLength = 0;
    char* pCursor = pBuffer;

    while (nLength < 0x100)
    {
        int nRead = Read(pCursor, 1);

        if (nRead != 0)
        {
            if (*pCursor == '\n' || *pCursor == '\r')
            {
                *pCursor = 0;

                do
                {
                    nRead = Read(&chSkipped, 1);
                } while (nRead != 0 && (chSkipped == '\n' || chSkipped == '\r'));

                if (nRead != 0)
                {
                    SeekRelative(-1, FILE_SEEK_CURRENT);
                }

                break;
            }

            pCursor++;
            nLength++;
        }
        else
        {
            if (nLength > 0 && nLength < 0xff)
            {
                pCursor[1] = 0;
            }

            break;
        }
    }

    *ppLine = pBuffer;

    return nLength;
}
