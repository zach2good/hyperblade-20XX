// Decodes every MDM1 file under a folder and checks each against the size its header records.
//     mdm1_check <folder> [<dump folder>]
// With a dump folder, each decoded stream is written there under its own name.

#include "common/compat.h"
#include "common/fileobj.h"
#include "graphics/mdm1.h"
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

int FileObj::Read(void* pBuffer, int nCount)
{
    return _read(m_fd, pBuffer, nCount);
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
        file.m_fd = _open(entry.path().string().c_str(), _O_BINARY | _O_RDONLY);

        unsigned int nMagic = 0;

        if (file.m_fd < 0)
        {
            continue;
        }

        if (file.Read(&nMagic, sizeof(nMagic)) == sizeof(nMagic) && nMagic == MDM1_MAGIC)
        {
            nChecked++;

            void* const pOut = MDM1::Decompress(&file);

            if (pOut == 0)
            {
                nFailed++;
                printf("FAIL %s\n", entry.path().string().c_str());
            }
            else if (argc > 2)
            {
                _lseek(file.m_fd, 4, SEEK_SET);

                int nSize = 0;
                file.Read(&nSize, sizeof(nSize));

                std::ofstream dump(std::filesystem::path(argv[2]) / entry.path().filename(), std::ios::binary);
                dump.write((const char*)pOut, nSize);
            }

            free(pOut);
        }

        _close(file.m_fd);
    }

    printf("%d MDM1 file(s), %d failed\n", nChecked, nFailed);

    if (nFailed != 0 || nChecked == 0)
    {
        return 1;
    }

    return 0;
}
