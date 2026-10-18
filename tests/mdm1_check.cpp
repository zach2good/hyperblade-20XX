// Decodes every MDM1 file under a folder and checks each against the size its header records.
//     mdm1_check <folder> [<dump folder>]
// With a dump folder, each decoded stream is written there under its own name.

#include "common/compat.h"
#include "common/fileobj.h"
#include "graphics/mdm1.h"
#include <filesystem>
#include <fstream>
#include <SDL3/SDL.h>
#include <cstdio>

int FileObj::Read(void* pBuffer, int nCount)
{
    return static_cast<int>(SDL_ReadIO(m_stream.As<SDL_IOStream>(), pBuffer, static_cast<size_t>(nCount)));
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: mdm1_check <folder> [<dump folder>]\n");
        return 2;
    }

    auto nChecked = 0;
    auto nFailed  = 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(argv[1]))
    {
        if (entry.is_regular_file() == false)
        {
            continue;
        }

        FileObj file;
        file.m_stream = FileObj::Stream{ SDL_IOFromFile(entry.path().string().c_str(), "rb") };

        unsigned int nMagic = 0;

        if (!file.m_stream)
        {
            continue;
        }

        if (file.Read(&nMagic, sizeof(nMagic)) == sizeof(nMagic) && nMagic == kMdm1Magic)
        {
            nChecked++;

            auto* const pOut = MDM1::Decompress(&file);

            if (pOut == nullptr)
            {
                nFailed++;
                printf("FAIL %s\n", entry.path().string().c_str());
            }
            else if (argc > 2)
            {
                SDL_SeekIO(file.m_stream.As<SDL_IOStream>(), 4, SDL_IO_SEEK_SET);

                auto nSize = 0;
                file.Read(&nSize, sizeof(nSize));

                std::ofstream dump(std::filesystem::path(argv[2]) / entry.path().filename(), std::ios::binary);
                dump.write(static_cast<const char*>(pOut), nSize);
            }

            free(pOut);
        }

        SDL_CloseIO(file.m_stream.As<SDL_IOStream>());
    }

    printf("%d MDM1 file(s), %d failed\n", nChecked, nFailed);

    if (nFailed != 0 || nChecked == 0)
    {
        return 1;
    }

    return 0;
}
