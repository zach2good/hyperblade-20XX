// MDM1 LZSS: sixteen flags a word; a reference is 12 bits of distance and 4 of length plus three.

#include "common/compat.h"
#include "common/fileobj.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>

#define MATCH_LENGTH_BIAS 3
#define MATCH_LENGTH_LONG 0x12
#define MATCH_LENGTH_END  0x10

#define WINDOW_MARGIN 0x80

#define WINDOW_BYTES 0x1000

// The window is freed before the size check that can discard the output.
// FUNCTION: HYPERX 0x00440140
void* MDM1::Decompress(FileObj* pFile)
{
    int nSize;

    auto* const pWindow = static_cast<unsigned char*>(malloc(WINDOW_BYTES));

    if (pWindow == nullptr)
    {
        return pWindow;
    }

    if (pFile->Read(&nSize, sizeof(nSize)) != sizeof(nSize))
    {
        return nullptr;
    }

    auto* const pOut = static_cast<unsigned char*>(malloc(nSize));

    if (pOut == nullptr)
    {
        free(pWindow);

        return nullptr;
    }

    const int nDecompressed = MDM1::LZSSDecompress(MDM1::ReadCallback, pFile, pOut, nSize, pWindow, WINDOW_BYTES);

    free(pWindow);

    if (nDecompressed != nSize)
    {
        free(pOut);

        return nullptr;
    }

    return pOut;
}

// FUNCTION: HYPERX 0x004401e0
int MDM1::ReadCallback(char* pBuffer, int nCount, FileObj* pFile)
{
    return pFile->Read(pBuffer, nCount);
}

// Decompresses one LZSS stream from the read callback into pOut through the sliding window, and
// returns the bytes written, or minus one when a read fails or the output would overrun. Each
// flag word governs sixteen items, low bit first: clear is a literal byte, set is a reference
// of 12 bits of distance and 4 of length. A long length that carries out ends the stream, except
// at 0x10, which copies nothing.
// FUNCTION: HYPERX 0x00493f80
int MDM1::LZSSDecompress(MDM1ReadFn pfnRead, FileObj* pFile, unsigned char* pOut, int nOutCapacity, unsigned char* pWindow, int nWindowSize)
{
    auto nAt  = nWindowSize;
    auto nEnd = nWindowSize;
    auto nOut = 0;

    // Moves the unread tail to the front and fills the rest; false when the read fails.
    const auto refill = [&]
    {
        const auto nTail = nEnd - nAt;

        memmove(pWindow, pWindow + nAt, nTail);

        const auto nRead = pfnRead(reinterpret_cast<char*>(pWindow) + nTail, nWindowSize - nTail, pFile);

        nAt  = 0;
        nEnd = nTail + (std::max)(nRead, 0);

        return nRead >= 0;
    };

    if (refill() == false || nEnd == 0)
    {
        return -1;
    }

    while (true)
    {
        // One flag word and its sixteen items never take more than the margin.
        if (nAt >= nWindowSize - WINDOW_MARGIN && refill() == false)
        {
            return -1;
        }

        if (nAt + 2 > nEnd)
        {
            return -1;
        }

        const unsigned int nFlags = pWindow[nAt] | (pWindow[nAt + 1] << 8);
        nAt += 2;

        for (int nBit = 0; nBit < 16; nBit++)
        {
            if ((nFlags & (1u << nBit)) == 0)
            {
                if (nAt >= nEnd || nOut >= nOutCapacity)
                {
                    return -1;
                }

                pOut[nOut++] = pWindow[nAt++];

                continue;
            }

            if (nAt + 2 > nEnd)
            {
                return -1;
            }

            const unsigned int nWord = pWindow[nAt] | (pWindow[nAt + 1] << 8);
            nAt += 2;

            const auto nFrom   = nOut - static_cast<int>(nWord >> 4);
            auto       nLength = static_cast<int>(nWord & 0xf) + MATCH_LENGTH_BIAS;

            if (nLength == MATCH_LENGTH_LONG)
            {
                if (nAt >= nEnd)
                {
                    return -1;
                }

                nLength += pWindow[nAt++];

                if (nLength > 0xff)
                {
                    if ((nLength & 0xff) == MATCH_LENGTH_END)
                    {
                        continue;
                    }

                    return nOut;
                }
            }

            if (nFrom < 0 || nOut + nLength > nOutCapacity)
            {
                return -1;
            }

            // Byte by byte: a reference may overlap what it is writing.
            for (int i = 0; i < nLength; i++)
            {
                pOut[nOut + i] = pOut[nFrom + i];
            }

            nOut += nLength;
        }
    }
}
