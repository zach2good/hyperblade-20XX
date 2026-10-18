#include "compat.h"
#include "common/path.h"
#include "fileobj.h"
#include "app/gameapp.h"
#include <SDL3/SDL.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

constexpr auto kTextMax = 0x100;

namespace
{

SDL_IOStream* IO(const FileObj& file)
{
    return file.m_stream.As<SDL_IOStream>();
}

// The original opened with _open's flags, so writing without truncating creates the file when it
// is missing and keeps it when it is there. A mode outside the three left the flags uninitialised.
SDL_IOStream* OpenStream(const char* pszPath, FileMode mode)
{
    const auto path = Path::Native(pszPath);

    switch (mode)
    {
        case FileMode::kWriteNoTruncate:
        {
            auto* const pExisting = SDL_IOFromFile(path.c_str(), "r+b");

            if (pExisting != nullptr)
            {
                return pExisting;
            }

            return SDL_IOFromFile(path.c_str(), "w+b");
        }

        case FileMode::kWrite:
            return SDL_IOFromFile(path.c_str(), "w+b");

        default:
            return SDL_IOFromFile(path.c_str(), "rb");
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
char FileObj::Open(const char* pszPath, FileMode mode)
{
    char szResolved[kMaxPathLength];

    m_stream = Stream{ OpenStream(pszPath, mode) };

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

        m_stream = Stream{ OpenStream(szResolved, mode) };

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
                return static_cast<SDL_IOWhence>(nOrigin);
        }
    }();

    return static_cast<int>(SDL_SeekIO(IO(*this), nOffset, whence));
}

// FUNCTION: HYPERX 0x004405b0
int FileObj::Write(const void* pBuf, unsigned int nCount)
{
    return static_cast<int>(SDL_WriteIO(IO(*this), pBuf, nCount));
}

// Each line feed gains a carriage return, so a line of line feeds overruns the second buffer.
// FUNCTION: HYPERX 0x004405d0
int FileObj::PrintFormatted(const char* pszFormat, ...)
{
    char    szText[kTextMax];
    char    szOut[kTextMax];
    va_list argList;

    va_start(argList, pszFormat);

    vsprintf(szText, pszFormat, argList);

    va_end(argList);

    auto  ch    = szText[0];
    char* pSrc  = szText;
    char* pDst  = szOut;
    auto  nLeft = kTextMax;

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

    return Write(szOut, static_cast<unsigned int>(strlen(szOut)));
}

// FUNCTION: HYPERX 0x00440670
int FileObj::Read(void* pBuffer, int nCount)
{
    return static_cast<int>(SDL_ReadIO(IO(*this), pBuffer, static_cast<size_t>(nCount)));
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

    return static_cast<int>(bClosed);
}

// FUNCTION: HYPERX 0x004406b0
int FileObj::GetNextLine(char** ppLine)
{
    char chSkipped;

    char* const pBuffer = m_szLine;

    memset(pBuffer, 0, sizeof(m_szLine));

    auto  nLength = 0;
    auto* pCursor = pBuffer;

    while (nLength < 0x100)
    {
        auto nRead = Read(pCursor, 1);

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
