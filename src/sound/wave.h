// The wave file readers the audio manager loads its samples with.

#ifndef HYPERBLADE_WAVE_H
#define HYPERBLADE_WAVE_H

namespace Wave
{

enum class Result
{
    OK,
    NO_FILE,
    BAD_FORMAT
};

struct Info
{
    unsigned int nSamplesPerSec = 0;
    unsigned int nChannels      = 0;
    unsigned int nBitsPerSample = 0;
    unsigned int nDataOffset    = 0;
    unsigned int nDataBytes     = 0;
};

// Anything but a PCM RIFF WAVE with a data chunk is a bad format.
Result ReadInfo(const char* pszPath, Info& info);

// False on a short read.
bool ReadSamples(const char* pszPath, unsigned int nOffset, unsigned int nBytes, void* pBuffer);

} // namespace Wave

#endif
