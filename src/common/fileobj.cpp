#include "compat.h"
#include "fileobj.h"
#include "app/gameapp.h"
#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define TEXT_MAX 0x100

namespace
{

SDL_IOStream* IO(const FileObj& file)
{
    return file.m_stream.As<SDL_IOStream>();
}

// The original opened with _open's flags, so writing without truncating creates the file when it
// is missing and keeps it when it is there. A mode outside the three left the flags uninitialised.
SDL_IOStream* OpenStream(const char* pszPath, int nMode)
{
    switch (nMode)
    {
        case FILE_MODE_WRITE_NO_TRUNCATE:
        {
            SDL_IOStream* const pExisting = SDL_IOFromFile(pszPath, "r+b");

            if (pExisting != nullptr)
            {
                return pExisting;
            }

            return SDL_IOFromFile(pszPath, "w+b");
        }

        case FILE_MODE_WRITE:
            return SDL_IOFromFile(pszPath, "w+b");

        default:
            return SDL_IOFromFile(pszPath, "rb");
    }
}

} // namespace

// Callers close twice, and close a failed open.
// FUNCTION: HYPERX 0x00440460
void FileObj::Close()
{
    if (m_stream)
    {
        SDL_CloseIO(IO(*this));
        m_stream = Stream{};
    }
}

// FUNCTION: HYPERX 0x00440470
char FileObj::Open(const char* pszPath, int nMode)
{
    char szResolved[MAX_PATH];

    m_stream = Stream{ OpenStream(pszPath, nMode) };

    if (!m_stream)
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

        m_stream = Stream{ OpenStream(szResolved, nMode) };

        if (!m_stream)
        {
            return 0;
        }
    }

    return 1;
}

// The loader's origins are one to three where SDL, as the C library, wants zero to two.
// FUNCTION: HYPERX 0x00440550
int FileObj::SeekRelative(long nOffset, int nOrigin)
{
    const auto whence = [&]
    {
        switch (nOrigin)
        {
            case FILE_SEEK_START:
                return SDL_IO_SEEK_SET;

            case FILE_SEEK_CURRENT:
                return SDL_IO_SEEK_CUR;

            case FILE_SEEK_END:
                return SDL_IO_SEEK_END;

            default:
                return (SDL_IOWhence)nOrigin;
        }
    }();

    return (int)SDL_SeekIO(IO(*this), nOffset, whence);
}

// FUNCTION: HYPERX 0x004405b0
int FileObj::Write(const void* pBuf, unsigned int nCount)
{
    return (int)SDL_WriteIO(IO(*this), pBuf, nCount);
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

    return Write(szOut, (unsigned int)strlen(szOut));
}

// FUNCTION: HYPERX 0x00440670
int FileObj::Read(void* pBuffer, int nCount)
{
    return (int)SDL_ReadIO(IO(*this), pBuffer, (size_t)nCount);
}

// FUNCTION: HYPERX 0x00440690
int FileObj::CloseAndTest()
{
    if (!m_stream)
    {
        return 0;
    }

    const auto bClosed = SDL_CloseIO(IO(*this));

    m_stream = Stream{};

    return (int)bClosed;
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
