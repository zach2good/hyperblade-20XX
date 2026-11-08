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

#ifdef HYPERX_V100
int Create(LPSTR pszPath, HMMIO* phmmio, WAVEFORMATEX* pFormat, MMCKINFO* pChunk, MMCKINFO* pRiff);
int StartDataWrite(HMMIO* phmmio, MMCKINFO* pChunk, MMIOINFO* pInfo);
int Write(HMMIO hmmio, unsigned int nBytes, const char* pData, MMCKINFO* pChunk, unsigned int* pnWritten, MMIOINFO* pInfo);
int CloseWrite(HMMIO* phmmio, MMCKINFO* pChunk, MMCKINFO* pRiff, MMIOINFO* pInfo, DWORD nSamples);
int CopyUselessChunks(HMMIO* phmmioIn, MMCKINFO* pChunk, MMCKINFO* pRiff, HMMIO* phmmioOut);
int CopyChunk(HMMIO hmmioSource, HMMIO hmmioDest, const MMCKINFO* pChunk);
int Load(LPSTR pszPath, unsigned int* pnBytes, DWORD* pnSamples, WAVEFORMATEX** ppFormat, char** ppData);
int Save(LPSTR pszPath, unsigned int nBytes, DWORD nSamples, WAVEFORMATEX* pFormat, const char* pData);
#endif

} // namespace Wave

#endif
