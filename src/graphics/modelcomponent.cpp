#include "common/compat.h"
#include "common/fileobj.h"
#include <new.h>
#include "globals.h"
#include "app/gameapp.h"
#include "modelcomponent.h"
#include "meshblock.h"
#ifdef HYPERX_V100
#include <stdio.h>
#include <string.h>
#include "graphics/meshmaterial.h"
#endif

struct ComponentFileHeader
{
    unsigned int cRecords;
    int          cBytes;
};

// FUNCTION: HYPERX100 0x0044f530
// FUNCTION: HYPERX 0x004361e0
ModelComponent* __cdecl ModelComponent::ReadFirst(FileObj* pFile)
{
    int                   nSize;
    ModelComponent* const pBlob = (ModelComponent*)ModelComponent::Read(pFile, &nSize);

    if (pBlob != 0)
    {
        pBlob->m_nSize = nSize;
    }

    return pBlob;
}

// The read error reports the identifier from the iteration before, or nought at the first record.
// FUNCTION: HYPERX100 0x0044fd00
// FUNCTION: HYPERX 0x004363a0
int __cdecl ModelComponent::ReadAll(FileObj* pFile, MeshBlock*** pppBlocks, unsigned short* pnCount)
{
    ComponentFileHeader header;
    int                 nSize;

    int nTotal = 0;
    if (pFile->Read(&header, sizeof(header)) != 0)
    {
        unsigned short iEntry  = 0;
        short          nLastId = 0;

        MeshBlock** const papBlocks = new MeshBlock*[header.cRecords];

        *pppBlocks = papBlocks;
        *pnCount   = (unsigned short)header.cRecords;

        while (iEntry != header.cRecords)
        {
            MeshBlock* const pBlock = (MeshBlock*)ModelComponent::Read(pFile, &nSize);

            if (pBlock == 0)
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

        if (nTotal == header.cBytes && iEntry == (short)header.cRecords)
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
// FUNCTION: HYPERX100 0x004509b0
// FUNCTION: HYPERX 0x00436d30
void* __cdecl ModelComponent::Read(FileObj* pFile, int* pnSize)
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

    return 0;
}

#ifdef HYPERX_V100
// FUNCTION: HYPERX100 0x0044f800
ModelComponent* __cdecl ModelComponent::ReadFirstOrFail(FileObj* pFile)
{
    int                   nSize;
    ModelComponent* const pBlob = (ModelComponent*)ModelComponent::Read(pFile, &nSize);

    if (pBlob != 0)
    {
        pBlob->m_nSize = nSize;

        return pBlob;
    }

    g_gameApp.Fatal("Error reading model component!");

    return pBlob;
}

// Reads a model's material block and prints it; each material's texture index is cleared as it goes.
// FUNCTION: HYPERX100 0x0044f860
void __cdecl ModelComponent::PrintMaterials(FileObj* pFile, FileObj* pOut)
{
    unsigned char nMaterials;
    MaterialName  szFileName;
    short         nTextures;

    if (pFile->Read(&nMaterials, sizeof(nMaterials)) == 0)
    {
        g_gameApp.Fatal("Error reading Material List count for printing!");
    }

    if (pFile->Read(szFileName, sizeof(szFileName)) == 0)
    {
        g_gameApp.Fatal("Error reading Material file name for printing!");
    }

    MaterialName* const pNames = (MaterialName*)operator new(nMaterials * sizeof(MaterialName));

    if (pFile->Read(pNames, nMaterials * sizeof(MaterialName)) == 0)
    {
        g_gameApp.Fatal("Error reading Material names for printing!");
    }

    MeshMaterial* const pMaterials = new MeshMaterial[nMaterials];

    if (pFile->Read(pMaterials, nMaterials * sizeof(MeshMaterial)) == 0)
    {
        g_gameApp.Fatal("Error reading Materials for printing!");
    }

    if (pFile->Read(&nTextures, sizeof(nTextures)) == 0)
    {
        g_gameApp.Fatal("Error reading texturecount for printing!");
    }

    MaterialName* pTextures = 0;

    if (nTextures != 0)
    {
        pTextures = (MaterialName*)operator new(nTextures * sizeof(MaterialName));

        if (pFile->Read(pTextures, nTextures * sizeof(MaterialName)) == 0)
        {
            g_gameApp.Fatal("Error reading texture names for printing!");
        }
    }

    pOut->PrintFormatted("\n\n>>>  % s MATERIALS  <<< \n\n", szFileName);

    for (int i = 0; i < nMaterials; i++)
    {
        MeshMaterial* const pMaterial = &pMaterials[i];

        pOut->PrintFormatted("\tnewMtl %s\n", pNames[i]);
        pOut->PrintFormatted("\t %d\n", pMaterial->nColour);

        if (pMaterial->nTextureIndex != 0)
        {
            pOut->PrintFormatted("\ttx %s\n", pTextures[pMaterial->nTextureIndex - 1]);
        }

        pOut->PrintFormatted("\t %f\n", pMaterial->flAmbient);
        pOut->PrintFormatted("\t %f\n", pMaterial->flDiffuse);
        pOut->PrintFormatted("\t %f\n", pMaterial->flEmissive);

        if (pMaterial->nBackIndex < 0)
        {
            pOut->PrintFormatted("\t bf NONE\n");
        }
        else
        {
            pOut->PrintFormatted("\t bf %s\n", pNames[pMaterial->nBackIndex]);
        }

        pMaterial->nTextureIndex = 0;
    }

    if (nTextures != 0)
    {
        operator delete(pTextures);
    }

    operator delete(pNames);

    delete[] pMaterials;
}

static const char* const s_apszKeywords[] = { "id", "to", "jp", "ms", "mw", "tv", "usemtl", "rm", "matXref" };

#define KEYWORD_COUNT 9

// Sets the one-based keyword number and answers the text after the keyword, or nought for none.
// FUNCTION: HYPERX100 0x0044fc40
const char* __cdecl ModelComponent::MatchKeyword(const char* pszLine, short* pnKeyword)
{
    char szWord[20];

    *pnKeyword = 0;

    short       nKeyword = 0;
    const int   nSkip    = strspn(pszLine, " \t\n");
    const char* pszWord  = pszLine + nSkip;

    for (;;)
    {
        sscanf(pszWord, "%s", szWord);

        if (_stricmp(szWord, s_apszKeywords[nKeyword]) == 0)
        {
            break;
        }

        nKeyword++;

        if (nKeyword >= KEYWORD_COUNT)
        {
            return 0;
        }
    }

    *pnKeyword = nKeyword + 1;

    return pszLine + strlen(s_apszKeywords[nKeyword]) + nSkip;
}
#endif
