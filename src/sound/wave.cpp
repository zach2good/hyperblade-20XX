// The wave file readers, Wave.cpp in the original by its own assertion text.

#include "common/compat.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "wave.h"
#include "globals.h"
#include "app/gameapp.h"

enum WaveError
{
    WAVE_ERR_OPEN      = 0xe100,
    WAVE_ERR_FORMAT    = 0xe101,
    WAVE_ERR_SHORT     = 0xe102,
    WAVE_ERR_TRUNCATED = 0xe103,
    WAVE_ERR_MEMORY    = 0xe000
};

// FUNCTION: HYPERX100 0x00432dd0
// FUNCTION: HYPERX 0x00493430
int Wave::Open(const char* pszPath, HMMIO* phmmio, WAVEFORMATEX** ppFormat, MMCKINFO* pParent)
{
    PCMWAVEFORMAT pcm;
    MMCKINFO      chunk;
    WORD          cbExtra;
    HMMIO         hmmio;
    int           nResult;

    *ppFormat = 0;

    hmmio = mmioOpenA((LPSTR)pszPath, 0, MMIO_ALLOCBUF);

    if (hmmio == 0)
    {
        nResult = WAVE_ERR_OPEN;

        goto error;
    }

    nResult = mmioDescend(hmmio, pParent, 0, 0);

    if (nResult != 0)
    {
        goto error;
    }

    if (pParent->ckid != FOURCC_RIFF || pParent->fccType != mmioFOURCC('W', 'A', 'V', 'E'))
    {
        nResult = WAVE_ERR_FORMAT;

        goto error;
    }

    chunk.ckid = mmioFOURCC('f', 'm', 't', ' ');

    nResult = mmioDescend(hmmio, &chunk, pParent, MMIO_FINDCHUNK);

    if (nResult != 0)
    {
        goto error;
    }

    if (chunk.cksize < sizeof(PCMWAVEFORMAT))
    {
        nResult = WAVE_ERR_FORMAT;

        goto error;
    }

    if (mmioRead(hmmio, (HPSTR)&pcm, sizeof(PCMWAVEFORMAT)) != sizeof(PCMWAVEFORMAT))
    {
        nResult = WAVE_ERR_SHORT;

        goto error;
    }

    if (pcm.wf.wFormatTag == WAVE_FORMAT_PCM)
    {
        cbExtra = 0;
    }
    else
    {
        if (mmioRead(hmmio, (HPSTR)&cbExtra, sizeof(cbExtra)) != sizeof(cbExtra))
        {
            nResult = WAVE_ERR_SHORT;

            goto error;
        }
    }

    *ppFormat = (WAVEFORMATEX*)malloc(cbExtra + sizeof(WAVEFORMATEX));

    if (*ppFormat == 0)
    {
        nResult = WAVE_ERR_MEMORY;

        goto error;
    }

    memcpy(*ppFormat, &pcm, sizeof(pcm));

    (*ppFormat)->cbSize = cbExtra;

    if (cbExtra != 0)
    {
        if (mmioRead(hmmio, (HPSTR)(*ppFormat + 1), cbExtra) != cbExtra)
        {
            nResult = WAVE_ERR_FORMAT;

            goto error;
        }
    }

    nResult = mmioAscend(hmmio, &chunk, 0);

    if (nResult != 0)
    {
        goto error;
    }

    goto cleanup;

error:
    if (*ppFormat != 0)
    {
        free(*ppFormat);

        *ppFormat = 0;
    }

    if (hmmio != 0)
    {
        mmioClose(hmmio, 0);

        hmmio = 0;
    }

cleanup:
    *phmmio = hmmio;

    return nResult;
}

// Seeks four bytes past the parent chunk's data offset, which skips the form type.
// FUNCTION: HYPERX100 0x00432ff0
// FUNCTION: HYPERX 0x004935f0
int Wave::FindDataChunk(HMMIO* phmmio, MMCKINFO* pChunk, MMCKINFO* pParent)
{
    if (mmioSeek(*phmmio, pParent->dwDataOffset + 4, SEEK_SET) == -1)
    {
        // STRING: HYPERX 0x004c8c84
        g_gameApp.Fatal("mmioSeek failure in Wave.cpp line 186");
    }

    pChunk->ckid = mmioFOURCC('d', 'a', 't', 'a');

    return mmioDescend(*phmmio, pChunk, pParent, MMIO_FINDCHUNK);
}

// A failing mmioGetInfo returns one, not MMIO's code.
// FUNCTION: HYPERX100 0x00433060
// FUNCTION: HYPERX 0x00493640
int Wave::ReadChunk(HMMIO hmmio, int nBytes, char* pBuffer, MMCKINFO* pChunk, int* pnRead)
{
    MMIOINFO     info;
    int          nResult;
    unsigned int nDone;
    unsigned int nData;

    nResult = (mmioGetInfo(hmmio, &info, 0) != 0);

    if (nResult != 0)
    {
        goto error;
    }

    nData = nBytes;

    if (nData > pChunk->cksize)
    {
        nData = pChunk->cksize;
    }

    pChunk->cksize -= nData;

    for (nDone = 0; nDone < nData; nDone++)
    {
        if (info.pchNext == info.pchEndRead)
        {
            nResult = mmioAdvance(hmmio, &info, 0);

            if (nResult != 0)
            {
                goto error;
            }

            if (info.pchNext == info.pchEndRead)
            {
                nResult = WAVE_ERR_TRUNCATED;

                goto error;
            }
        }

        pBuffer[nDone] = *info.pchNext++;
    }

    nResult = mmioSetInfo(hmmio, &info, 0);

    if (nResult != 0)
    {
        goto error;
    }

    *pnRead = nData;

    goto done;

error:
    *pnRead = 0;

done:
    return nResult;
}

// FUNCTION: HYPERX100 0x00433200
// FUNCTION: HYPERX 0x00493710
int Wave::Close(HMMIO* phmmio, WAVEFORMATEX** ppFormat)
{
    if (*ppFormat != 0)
    {
        free(*ppFormat);

        *ppFormat = 0;
    }

    if (*phmmio != 0)
    {
        mmioClose(*phmmio, 0);

        *phmmio = 0;
    }

    return 0;
}

#ifdef HYPERX_V100
#define WAVE_ERR_WRITE 0xe104

// The fact chunk's placeholder, which CloseWrite replaces with the sample count.
#define WAVE_UNKNOWN_SAMPLES 0xffffffff

// FUNCTION: HYPERX100 0x00433250
int Wave::Create(LPSTR pszPath, HMMIO* phmmio, WAVEFORMATEX* pFormat, MMCKINFO* pChunk, MMCKINFO* pRiff)
{
    DWORD    nFactSamples = WAVE_UNKNOWN_SAMPLES;
    MMCKINFO fact;
    int      nResult;

    *phmmio = mmioOpenA(pszPath, 0, MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_CREATE);

    if (*phmmio == 0)
    {
        return WAVE_ERR_WRITE;
    }

    pRiff->fccType = mmioFOURCC('W', 'A', 'V', 'E');
    pRiff->cksize  = 0;

    nResult = mmioCreateChunk(*phmmio, pRiff, MMIO_CREATERIFF);

    if (nResult != 0)
    {
        return nResult;
    }

    pChunk->cksize = sizeof(PCMWAVEFORMAT);
    pChunk->ckid   = mmioFOURCC('f', 'm', 't', ' ');

    nResult = mmioCreateChunk(*phmmio, pChunk, 0);

    if (nResult != 0)
    {
        return nResult;
    }

    if (pFormat->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (mmioWrite(*phmmio, (char*)pFormat, sizeof(PCMWAVEFORMAT)) != sizeof(PCMWAVEFORMAT))
        {
            return WAVE_ERR_WRITE;
        }
    }
    else
    {
        const int nFormatBytes = sizeof(WAVEFORMATEX) + pFormat->cbSize;

        if (mmioWrite(*phmmio, (char*)pFormat, nFormatBytes) != nFormatBytes)
        {
            return WAVE_ERR_WRITE;
        }
    }

    nResult = mmioAscend(*phmmio, pChunk, 0);

    if (nResult != 0)
    {
        return nResult;
    }

    fact.ckid   = mmioFOURCC('f', 'a', 'c', 't');
    fact.cksize = 0;

    nResult = mmioCreateChunk(*phmmio, &fact, 0);

    if (nResult != 0)
    {
        return nResult;
    }

    if (mmioWrite(*phmmio, (char*)&nFactSamples, sizeof(nFactSamples)) != sizeof(nFactSamples))
    {
        return WAVE_ERR_WRITE;
    }

    if (mmioAscend(*phmmio, &fact, 0) != 0)
    {
        return WAVE_ERR_WRITE;
    }

    return 0;
}

// FUNCTION: HYPERX100 0x004333f0
int Wave::StartDataWrite(HMMIO* phmmio, MMCKINFO* pChunk, MMIOINFO* pInfo)
{
    int nResult;

    pChunk->cksize = 0;
    pChunk->ckid   = mmioFOURCC('d', 'a', 't', 'a');

    nResult = mmioCreateChunk(*phmmio, pChunk, 0);

    if (nResult == 0)
    {
        nResult = mmioGetInfo(*phmmio, pInfo, 0);
    }

    return nResult;
}

// The chunk is unread.
// FUNCTION: HYPERX100 0x00433440
int Wave::Write(HMMIO hmmio, unsigned int nBytes, const char* pData, MMCKINFO*, unsigned int* pnWritten, MMIOINFO* pInfo)
{
    int nResult = 0;

    *pnWritten = 0;

    for (unsigned int nDone = 0; nDone < nBytes; nDone++)
    {
        if (pInfo->pchNext == pInfo->pchEndWrite)
        {
            pInfo->dwFlags |= MMIO_DIRTY;

            nResult = mmioAdvance(hmmio, pInfo, MMIO_WRITE);

            if (nResult != 0)
            {
                break;
            }
        }

        *pInfo->pchNext++ = pData[nDone];

        (*pnWritten)++;
    }

    return nResult;
}

// FUNCTION: HYPERX100 0x004334c0
int Wave::CloseWrite(HMMIO* phmmio, MMCKINFO* pChunk, MMCKINFO* pRiff, MMIOINFO* pInfo, DWORD nSamples)
{
    int nResult;

    if (*phmmio == 0)
    {
        return 0;
    }

    pInfo->dwFlags |= MMIO_DIRTY;

    nResult = mmioSetInfo(*phmmio, pInfo, 0);

    if (nResult != 0)
    {
        goto error;
    }

    nResult = mmioAscend(*phmmio, pChunk, 0);

    if (nResult != 0)
    {
        goto error;
    }

    nResult = mmioAscend(*phmmio, pRiff, 0);

    if (nResult != 0)
    {
        goto error;
    }

    mmioSeek(*phmmio, 0, SEEK_SET);

    nResult = mmioDescend(*phmmio, pRiff, 0, 0);

    if (nResult != 0)
    {
        goto error;
    }

    pChunk->ckid = mmioFOURCC('f', 'a', 'c', 't');

    if (mmioDescend(*phmmio, pChunk, pRiff, MMIO_FINDCHUNK) == 0)
    {
        mmioWrite(*phmmio, (char*)&nSamples, sizeof(nSamples));
        mmioAscend(*phmmio, pChunk, 0);
    }
    else
    {
        // STRING: HYPERX100 0x0050f19c
        g_gameApp.Fatal("mmioDescend failure in Wave.cpp line 589");
    }

    nResult = mmioAscend(*phmmio, pRiff, 0);

error:
    if (*phmmio != 0)
    {
        mmioClose(*phmmio, 0);

        *phmmio = 0;
    }

    return nResult;
}

// Copies the DISP and plst chunks, then leaves the file just past the parent's form type.
// FUNCTION: HYPERX100 0x00433600
int Wave::CopyUselessChunks(HMMIO* phmmioIn, MMCKINFO* pChunk, MMCKINFO* pRiff, HMMIO* phmmioOut)
{
    int nResult;

    if (mmioSeek(*phmmioIn, pRiff->dwDataOffset + 4, SEEK_SET) == -1)
    {
        nResult = WAVE_ERR_SHORT;
    }
    else
    {
        nResult = 0;

        while (mmioDescend(*phmmioIn, pChunk, pRiff, 0) == 0 && pChunk->cksize + pChunk->dwDataOffset <= pRiff->cksize + pRiff->dwDataOffset)
        {
            switch (pChunk->ckid)
            {
                case mmioFOURCC('f', 'a', 'c', 't'):
                    break;

                case mmioFOURCC('D', 'I', 'S', 'P'):
                case mmioFOURCC('p', 'l', 's', 't'):
                    Wave::CopyChunk(*phmmioIn, *phmmioOut, pChunk);
                    break;

                default:
                    break;
            }

            mmioAscend(*phmmioIn, pChunk, 0);
        }
    }

    mmioSeek(*phmmioIn, pRiff->dwDataOffset + 4, SEEK_SET);

    return nResult;
}

// Answers one when the chunk was copied whole.
// FUNCTION: HYPERX100 0x00433700
int Wave::CopyChunk(HMMIO hmmioSource, HMMIO hmmioDest, const MMCKINFO* pChunk)
{
    MMCKINFO chunk;
    HPSTR    pBuffer = (HPSTR)malloc(pChunk->cksize);

    if (pBuffer == 0)
    {
        return 0;
    }

    chunk.ckid   = pChunk->ckid;
    chunk.cksize = pChunk->cksize;

    if (mmioCreateChunk(hmmioDest, &chunk, 0) == 0)
    {
        if ((DWORD)mmioRead(hmmioSource, pBuffer, pChunk->cksize) == pChunk->cksize)
        {
            if ((DWORD)mmioWrite(hmmioDest, pBuffer, pChunk->cksize) == pChunk->cksize)
            {
                if (mmioAscend(hmmioDest, &chunk, 0) == 0)
                {
                    if (pBuffer != 0)
                    {
                        free(pBuffer);
                    }

                    return 1;
                }
            }
        }
    }

    if (pBuffer != 0)
    {
        free(pBuffer);
    }

    return 0;
}

// The sample count is unread.
// FUNCTION: HYPERX100 0x004337e0
int Wave::Load(LPSTR pszPath, unsigned int* pnBytes, DWORD*, WAVEFORMATEX** ppFormat, char** ppData)
{
    HMMIO    hmmio;
    int      nRead;
    MMCKINFO chunk;
    MMCKINFO riff;
    int      nResult;

    *ppData   = 0;
    *ppFormat = 0;
    *pnBytes  = 0;

    nResult = Wave::Open(pszPath, &hmmio, ppFormat, &riff);

    if (nResult != 0)
    {
        goto error;
    }

    nResult = Wave::FindDataChunk(&hmmio, &chunk, &riff);

    if (nResult != 0)
    {
        goto error;
    }

    *ppData = (char*)malloc(chunk.cksize);

    if (*ppData == 0)
    {
        nResult = WAVE_ERR_MEMORY;

        goto error;
    }

    nResult = Wave::ReadChunk(hmmio, chunk.cksize, *ppData, &chunk, &nRead);

    if (nResult != 0)
    {
        goto error;
    }

    *pnBytes = nRead;

    goto done;

error:
    if (*ppData != 0)
    {
        free(*ppData);

        *ppData = 0;
    }

    if (*ppFormat != 0)
    {
        free(*ppFormat);

        *ppFormat = 0;
    }

done:
    if (hmmio != 0)
    {
        mmioClose(hmmio, 0);
    }

    return nResult;
}

// FUNCTION: HYPERX100 0x00433910
int Wave::Save(LPSTR pszPath, unsigned int nBytes, DWORD nSamples, WAVEFORMATEX* pFormat, const char* pData)
{
    HMMIO        hmmio;
    unsigned int nWritten;
    MMCKINFO     chunk;
    MMCKINFO     riff;
    MMIOINFO     info;
    int          nResult;

    nResult = Wave::Create(pszPath, &hmmio, pFormat, &chunk, &riff);

    if (nResult != 0)
    {
        return nResult;
    }

    nResult = Wave::StartDataWrite(&hmmio, &chunk, &info);

    if (nResult != 0)
    {
        return nResult;
    }

    nResult = Wave::Write(hmmio, nBytes, pData, &chunk, &nWritten, &info);

    if (nResult != 0)
    {
        return nResult;
    }

    return Wave::CloseWrite(&hmmio, &chunk, &riff, &info, nSamples);
}
#endif
