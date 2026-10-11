// The wave file readers the audio manager loads its samples with.

#ifndef HYPERBLADE_WAVE_H
#define HYPERBLADE_WAVE_H
#include "common/compat.h"
#include <mmsystem.h>

namespace Wave
{

int Open(const char* pszPath, HMMIO* phmmio, WAVEFORMATEX** ppFormat, MMCKINFO* pParent);
int FindDataChunk(HMMIO* phmmio, MMCKINFO* pChunk, MMCKINFO* pParent);
int ReadChunk(HMMIO hmmio, int nBytes, char* pBuffer, MMCKINFO* pChunk, int* pnRead);
int Close(HMMIO* phmmio, WAVEFORMATEX** ppFormat);

} // namespace Wave

#endif
