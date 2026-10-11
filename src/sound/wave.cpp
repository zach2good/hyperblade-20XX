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
