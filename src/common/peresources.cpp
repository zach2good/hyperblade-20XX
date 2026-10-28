#include "peresources.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>

namespace
{

constexpr auto kSubdirectory   = 0x80000000u;
constexpr auto kNamedEntry     = 0x80000000u;
constexpr auto kMaxResourceDir = 4096u;

// Little-endian reads that mark the image bad instead of reading past its end.
struct Image
{
    std::vector<unsigned char> bytes;
    bool                       bValid = true;

    unsigned int U16(size_t nAt)
    {
        if (nAt + 2 > bytes.size())
        {
            bValid = false;

            return 0;
        }

        return static_cast<unsigned int>(bytes[nAt] | bytes[nAt + 1] << 8);
    }

    unsigned int U32(size_t nAt)
    {
        return U16(nAt) | U16(nAt + 2) << 16;
    }
};

} // namespace

std::optional<std::vector<PEResources::Resource> > PEResources::Read(const std::string& strExe)
{
    auto file = std::ifstream(strExe, std::ios::binary);

    if (!file)
    {
        return std::nullopt;
    }

    auto image = Image{ std::vector<unsigned char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()) };

    const auto nPE = static_cast<size_t>(image.U32(0x3c));

    if (image.U32(nPE) != 0x00004550)
    {
        return std::nullopt;
    }

    const auto nCOFF     = nPE + 4;
    const auto nSections = image.U16(nCOFF + 2);
    const auto nOptional = nCOFF + 20;

    const auto nDirectories = [&]
    {
        // PE32+ widens the fields before the data directories.
        if (image.U16(nOptional) == 0x20b)
        {
            return nOptional + 112;
        }

        return nOptional + 96;
    }();

    const auto nSectionTable = nOptional + image.U16(nCOFF + 16);

    const auto toOffset = [&](unsigned int nRVA) -> size_t
    {
        for (unsigned int nSection = 0; nSection < nSections; nSection++)
        {
            const auto nAt      = nSectionTable + nSection * 40;
            const auto nAddress = image.U32(nAt + 12);
            const auto nSize    = (std::max)(image.U32(nAt + 8), image.U32(nAt + 16));

            if (nRVA >= nAddress && nRVA - nAddress < nSize)
            {
                return image.U32(nAt + 20) + static_cast<size_t>(nRVA - nAddress);
            }
        }

        image.bValid = false;

        return 0;
    };

    const auto nRoot = toOffset(image.U32(nDirectories + 2 * 8));

    // Each directory entry is an id, or a name's offset from the resource root, and an offset.
    const auto entries = [&](size_t nDirectory)
    {
        auto found = std::vector<std::pair<unsigned int, unsigned int> >();

        const auto nCount = (std::min)(image.U16(nDirectory + 12) + image.U16(nDirectory + 14), kMaxResourceDir);

        for (unsigned int n = 0; n < nCount && image.bValid; n++)
        {
            found.emplace_back(image.U32(nDirectory + 16 + n * 8), image.U32(nDirectory + 20 + n * 8));
        }

        return found;
    };

    // A resource name is a counted UTF-16 string. The game's are ASCII.
    const auto name = [&](unsigned int nEntry)
    {
        auto       strName = std::string();
        const auto nAt     = nRoot + (nEntry & ~kNamedEntry);
        const auto nLength = image.U16(nAt);

        for (unsigned int n = 0; n < nLength && image.bValid; n++)
        {
            strName.push_back(static_cast<char>(image.U16(nAt + 2 + n * 2)));
        }

        return strName;
    };

    auto resources = std::vector<Resource>();

    for (const auto& [nType, nTypeAt] : entries(nRoot))
    {
        if ((nType & kNamedEntry) != 0 || (nTypeAt & kSubdirectory) == 0)
        {
            continue;
        }

        for (const auto& [nName, nNameAt] : entries(nRoot + (nTypeAt & ~kSubdirectory)))
        {
            if ((nNameAt & kSubdirectory) == 0)
            {
                continue;
            }

            const auto languages = entries(nRoot + (nNameAt & ~kSubdirectory));

            if (languages.empty() || (languages[0].second & kSubdirectory) != 0)
            {
                continue;
            }

            const auto nEntry = nRoot + languages[0].second;
            const auto nAt    = toOffset(image.U32(nEntry));
            const auto nSize  = image.U32(nEntry + 4);

            if (!image.bValid || nAt + nSize > image.bytes.size())
            {
                return std::nullopt;
            }

            auto resource = Resource{ nType, 0, std::string(), std::vector<unsigned char>(image.bytes.begin() + nAt, image.bytes.begin() + nAt + nSize) };

            if ((nName & kNamedEntry) != 0)
            {
                resource.strName = name(nName);
            }
            else
            {
                resource.nID = nName;
            }

            resources.push_back(std::move(resource));
        }
    }

    if (!image.bValid)
    {
        return std::nullopt;
    }

    return resources;
}
