#include "common/compat.h"
#include "common/fileobj.h"
#include <new>
#include "globals.h"
#include "app/gameapp.h"
#include "modelcomponent.h"
#include "meshblock.h"

struct ComponentFileHeader
{
    unsigned int cRecords;
    int          cBytes;
};

// FUNCTION: HYPERX 0x004361e0
ModelComponent* ModelComponent::ReadFirst(FileObj* pFile)
{
    int         nSize;
    auto* const pBlob = static_cast<ModelComponent*>(ModelComponent::Read(pFile, &nSize));

    if (pBlob != nullptr)
    {
        pBlob->m_nSize = nSize;
    }

    return pBlob;
}

// The read error reports the identifier from the iteration before, or nought at the first record.
// FUNCTION: HYPERX 0x004363a0
int ModelComponent::ReadAll(FileObj* pFile, MeshBlock*** pppBlocks, unsigned short* pnCount)
{
    ComponentFileHeader header;
    int                 nSize;

    auto nTotal = 0;
    if (pFile->Read(&header, sizeof(header)) != 0)
    {
        unsigned short iEntry  = 0;
        short          nLastId = 0;

        auto** const papBlocks = new MeshBlock*[header.cRecords];

        *pppBlocks = papBlocks;
        *pnCount   = static_cast<unsigned short>(header.cRecords);

        while (iEntry != header.cRecords)
        {
            auto* const pBlock = static_cast<MeshBlock*>(ModelComponent::Read(pFile, &nSize));

            if (pBlock == nullptr)
            {
                pFile->CloseAndTest();

                // STRING: HYPERX 0x004be078
                g_gameApp.Fatal("Read error loading ModelComponents\n Last component read %d!", nLastId);

                return 0;
            }

            pBlock->nSize = nSize;

            papBlocks[iEntry] = pBlock;

            nLastId = pBlock->nSlot;

            nTotal += nSize;
            iEntry++;
        }

        if (nTotal == header.cBytes && iEntry == static_cast<short>(header.cRecords))
        {
            return nTotal;
        }

        pFile->CloseAndTest();

        // STRING: HYPERX 0x004be018
        g_gameApp.Fatal("Component file totals error.\nRecord count expected: %d  found %d\nBytes expected: %d found %d!", header.cRecords, iEntry, header.cBytes, nTotal);

        return 0;
    }

    pFile->CloseAndTest();

    // STRING: HYPERX 0x004bdfe8
    g_gameApp.Fatal("Read error loading component file record count!");

    return 0;
}

// The block's first word is left to the caller.
// FUNCTION: HYPERX 0x00436d30
void* ModelComponent::Read(FileObj* pFile, int* pnSize)
{
    int nSize;

    if (pFile->Read(&nSize, sizeof(nSize)) != 0)
    {
        *pnSize = nSize;

        char* const pBlock = new char[nSize];

        if (pFile->Read(pBlock + sizeof(nSize), *pnSize - sizeof(nSize)) != 0)
        {
            return pBlock;
        }

        delete[] pBlock;
    }

    return nullptr;
}
