#include "common/pak.h"
#include "stb_image.h"
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <string>

namespace
{

constexpr auto END_SIGNATURE     = 0x06054b50u;
constexpr auto CENTRAL_SIGNATURE = 0x02014b50u;
constexpr auto LOCAL_SIGNATURE   = 0x04034b50u;
constexpr auto END_SIZE          = 22u;
constexpr auto CENTRAL_SIZE      = 46u;
constexpr auto LOCAL_SIZE        = 30u;
constexpr auto MAX_COMMENT       = 0xffffu;
constexpr auto METHOD_STORED     = 0;
constexpr auto METHOD_DEFLATED   = 8;
constexpr auto MAX_DEFLATE_RATIO = 1032u;

struct Entry
{
    std::uint16_t nMethod      = 0;
    std::uint32_t nPacked      = 0;
    std::uint32_t nSize        = 0;
    std::uint32_t nLocalOffset = 0;
};

std::vector<unsigned char>                 s_bytes;
std::map<std::string, Entry, std::less<> > s_entries;

bool Fits(std::size_t nOffset, std::size_t nLength)
{
    return nOffset <= s_bytes.size() && nLength <= s_bytes.size() - nOffset;
}

// Little-endian. Callers check Fits first.
std::uint32_t ReadU16(std::size_t nOffset)
{
    return s_bytes[nOffset] | (s_bytes[nOffset + 1] << 8);
}

std::uint32_t ReadU32(std::size_t nOffset)
{
    return ReadU16(nOffset) | (ReadU16(nOffset + 2) << 16);
}

// The end record sits last, after a comment of up to 64KB.
bool FindEnd(std::size_t* pnOffset)
{
    if (s_bytes.size() < END_SIZE)
    {
        return false;
    }

    const auto nLast  = s_bytes.size() - END_SIZE;
    const auto nFirst = [&]() -> std::size_t
    {
        if (nLast > MAX_COMMENT)
        {
            return nLast - MAX_COMMENT;
        }

        return 0;
    }();

    for (auto nOffset = nLast + 1; nOffset-- > nFirst;)
    {
        if (ReadU32(nOffset) == END_SIGNATURE)
        {
            *pnOffset = nOffset;

            return true;
        }
    }

    return false;
}

bool ReadDirectory()
{
    auto nEnd = (std::size_t)0;

    if (!FindEnd(&nEnd))
    {
        return false;
    }

    const auto nCount = ReadU16(nEnd + 10);

    auto nOffset = (std::size_t)ReadU32(nEnd + 16);

    for (auto i = 0u; i < nCount; i++)
    {
        if (!Fits(nOffset, CENTRAL_SIZE) || ReadU32(nOffset) != CENTRAL_SIGNATURE)
        {
            return false;
        }

        const auto nNameLength = ReadU16(nOffset + 28);
        const auto nSkipLength = ReadU16(nOffset + 30) + ReadU16(nOffset + 32);

        if (!Fits(nOffset + CENTRAL_SIZE, nNameLength + nSkipLength))
        {
            return false;
        }

        const auto* const pName = (const char*)s_bytes.data() + nOffset + CENTRAL_SIZE;

        s_entries[std::string(pName, nNameLength)] = Entry{
            (std::uint16_t)ReadU16(nOffset + 10),
            ReadU32(nOffset + 20),
            ReadU32(nOffset + 24),
            ReadU32(nOffset + 42),
        };

        nOffset += CENTRAL_SIZE + nNameLength + nSkipLength;
    }

    return true;
}

} // namespace

bool Pak::Open(const char* pszPath)
{
    auto file = std::ifstream(pszPath, std::ios::binary);

    s_bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    s_entries.clear();

    return file.is_open() && ReadDirectory();
}

std::vector<unsigned char> Pak::Read(std::string_view name)
{
    const auto found = s_entries.find(name);

    if (found == s_entries.end())
    {
        return {};
    }

    const auto& entry = found->second;

    if (!Fits(entry.nLocalOffset, LOCAL_SIZE) || ReadU32(entry.nLocalOffset) != LOCAL_SIGNATURE)
    {
        return {};
    }

    const auto nData = (std::size_t)entry.nLocalOffset + LOCAL_SIZE + ReadU16(entry.nLocalOffset + 26) + ReadU16(entry.nLocalOffset + 28);

    if (!Fits(nData, entry.nPacked))
    {
        return {};
    }

    const auto* const pPacked = s_bytes.data() + nData;

    if (entry.nMethod == METHOD_STORED && entry.nPacked == entry.nSize)
    {
        return std::vector<unsigned char>(pPacked, pPacked + entry.nPacked);
    }

    // Deflate packs at most 1032 to one, so a bigger size is a damaged pak.
    if (entry.nMethod != METHOD_DEFLATED || entry.nSize / MAX_DEFLATE_RATIO > entry.nPacked)
    {
        return {};
    }

    auto bytes = std::vector<unsigned char>(entry.nSize);

    const auto nUnpacked = stbi_zlib_decode_noheader_buffer((char*)bytes.data(), (int)bytes.size(), (const char*)pPacked, (int)entry.nPacked);

    if (nUnpacked != (int)entry.nSize)
    {
        return {};
    }

    return bytes;
}
