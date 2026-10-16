#include "wave.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace
{

constexpr auto WAVE_FORMAT_PCM_TAG = 1;

// Far more chunks than any of the game's files has, so a damaged file cannot loop for ever.
constexpr auto MAX_CHUNKS = 64;

std::uint32_t ReadU32(const unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((std::uint32_t)p[3] << 24);
}

std::uint16_t ReadU16(const unsigned char* p)
{
    return (std::uint16_t)(p[0] | (p[1] << 8));
}

class File
{
public:
    explicit File(const char* pszPath)
    : m_pFile(std::fopen(pszPath, "rb"))
    {
    }

    ~File()
    {
        if (m_pFile != nullptr)
        {
            std::fclose(m_pFile);
        }
    }

    File(const File&)            = delete;
    File& operator=(const File&) = delete;

    std::FILE* Get() const
    {
        return m_pFile;
    }

private:
    std::FILE* m_pFile;
};

} // namespace

// FUNCTION: HYPERX 0x00493430
Wave::Result Wave::ReadInfo(const char* pszPath, Info& info)
{
    const auto file = File(pszPath);

    if (file.Get() == nullptr)
    {
        return Result::NO_FILE;
    }

    unsigned char aHeader[12];

    if (std::fread(aHeader, 1, sizeof(aHeader), file.Get()) != sizeof(aHeader) || std::memcmp(aHeader, "RIFF", 4) != 0 || std::memcmp(aHeader + 8, "WAVE", 4) != 0)
    {
        return Result::BAD_FORMAT;
    }

    auto bFormat = false;
    auto bData   = false;
    auto nOffset = std::uint32_t{ sizeof(aHeader) };

    for (int nChunk = 0; nChunk < MAX_CHUNKS && !(bFormat && bData); nChunk++)
    {
        unsigned char aChunk[8];

        if (std::fseek(file.Get(), (long)nOffset, SEEK_SET) != 0 || std::fread(aChunk, 1, sizeof(aChunk), file.Get()) != sizeof(aChunk))
        {
            return Result::BAD_FORMAT;
        }

        const auto nSize = ReadU32(aChunk + 4);

        if (std::memcmp(aChunk, "fmt ", 4) == 0)
        {
            unsigned char aFormat[16];

            if (nSize < sizeof(aFormat) || std::fread(aFormat, 1, sizeof(aFormat), file.Get()) != sizeof(aFormat) || ReadU16(aFormat) != WAVE_FORMAT_PCM_TAG)
            {
                return Result::BAD_FORMAT;
            }

            info.nChannels      = ReadU16(aFormat + 2);
            info.nSamplesPerSec = ReadU32(aFormat + 4);
            info.nBitsPerSample = ReadU16(aFormat + 14);

            bFormat = true;
        }
        else if (std::memcmp(aChunk, "data", 4) == 0)
        {
            info.nDataOffset = nOffset + sizeof(aChunk);
            info.nDataBytes  = nSize;

            bData = true;
        }

        // Chunks start on even offsets.
        nOffset += sizeof(aChunk) + nSize + (nSize & 1);
    }

    if (bFormat && bData)
    {
        return Result::OK;
    }

    return Result::BAD_FORMAT;
}

bool Wave::ReadSamples(const char* pszPath, unsigned int nOffset, unsigned int nBytes, void* pBuffer)
{
    const auto file = File(pszPath);

    return file.Get() != nullptr && std::fseek(file.Get(), (long)nOffset, SEEK_SET) == 0 && std::fread(pBuffer, 1, nBytes, file.Get()) == nBytes;
}
