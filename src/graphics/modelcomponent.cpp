#include "common/compat.h"
#include "common/fileobj.h"
#include "globals.h"
#include "app/gameapp.h"
#include "modelcomponent.h"
#include "meshblock.h"
#include <SDL3/SDL.h>

struct ComponentFileHeader
{
    unsigned int cRecords;
    int          cBytes;
};

// FUNCTION: HYPERX 0x004361e0
Record<ModelComponent> ModelComponent::ReadFirst(FileObj* pFile)
{
    int  nSize;
    auto pBlob = ModelComponent::Read<ModelComponent>(pFile, &nSize);

    if (pBlob != nullptr)
    {
        pBlob->m_nSize = nSize;
    }

    return pBlob;
}

// The original met a damaged record with a fatal error, and MDLSHD's NEMPTM.WBM ships with one.
// FUNCTION: HYPERX 0x004363a0
bool ModelComponent::ReadAll(FileObj* pFile, std::vector<Record<MeshBlock> >* pBlocks)
{
    ComponentFileHeader header;
    int                 nSize;

    auto nTotal = 0;
    if (pFile->Read(&header, sizeof(header)) != 0)
    {
        unsigned short iEntry = 0;

        const auto nStart = pFile->SeekRelative(0, FileSeekOrigin::kCurrent);

        pBlocks->clear();
        pBlocks->reserve(header.cRecords);

        while (iEntry != header.cRecords)
        {
            auto pBlock = ModelComponent::Read<MeshBlock>(pFile, &nSize);

            if (pBlock == nullptr)
            {
                SDL_Log("Model component %d of %u is damaged", iEntry, header.cRecords);

                pFile->SeekRelative(nStart + header.cBytes, FileSeekOrigin::kStart);

                return false;
            }

            pBlock->nSize = nSize;

            pBlocks->push_back(std::move(pBlock));

            nTotal += nSize;
            iEntry++;
        }

        if (nTotal == header.cBytes && iEntry == static_cast<short>(header.cRecords))
        {
            return true;
        }

        pFile->CloseAndTest();

        // STRING: HYPERX 0x004be018
        g_gameApp.Fatal("Component file totals error.\nRecord count expected: %d  found %d\nBytes expected: %d found %d!", header.cRecords, iEntry, header.cBytes, nTotal);

        return false;
    }

    pFile->CloseAndTest();

    // STRING: HYPERX 0x004bdfe8
    g_gameApp.Fatal("Read error loading component file record count!");

    return false;
}

// The block's first word is left to the caller.
// FUNCTION: HYPERX 0x00436d30
template <class T>
Record<T> ModelComponent::Read(FileObj* pFile, int* pnSize)
{
    int nSize;

    // A record must hold its own size, and must not run past the end of the file.
    if (pFile->Read(&nSize, sizeof(nSize)) != 0 && nSize >= static_cast<int>(sizeof(nSize)))
    {
        *pnSize = nSize;

        auto pBlock = MakeRecord<T>(nSize);

        const auto nBody = nSize - static_cast<int>(sizeof(nSize));

        if (pFile->Read(reinterpret_cast<char*>(pBlock.get()) + sizeof(nSize), nBody) == nBody)
        {
            return pBlock;
        }
    }

    return nullptr;
}
