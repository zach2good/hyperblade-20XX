#include "common/compat.h"
#include "graphics/hwrenderer.h"
#include "modeldatabase.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "lodnode.h"
#include "texcacheentry.h"
#include "meshblock.h"
#include "graphics/meshmaterial.h"
#include "graphics/modelcomponent.h"
#include "common/fileobj.h"
#include "graphics/transformedvertex.h"
#include "common/pathstring.h"
#include "math/vector.h"
#include <new>
#include <cstring>
#include "app/gameapp.h"
#include "graphics/modelfile.h"
#include "graphics/meshbone.h"
#include <SDL3/SDL.h>

// Frees every node whatever its reference count.
// FUNCTION: HYPERX 0x00472830
ModelDatabase::~ModelDatabase()
{
    while (m_pListHead != nullptr)
    {
        auto* const pNext = m_pListHead->m_pNext;
        auto* const pNode = m_pListHead;

        delete pNode;

        m_pListHead = pNext;
    }

    m_pListTail = nullptr;
    m_pListHead = nullptr;

    if (g_pModelVertexBuf != nullptr)
    {
        delete[] g_pModelVertexBuf;
        g_pModelVertexBuf = nullptr;
    }

    if (g_ppFaceList != nullptr)
    {
        delete[] g_ppFaceList;
        g_ppFaceList = nullptr;
    }

    if (g_pFaceRecords != nullptr)
    {
        delete[] g_pFaceRecords;
        g_pFaceRecords = nullptr;
    }

    auto* pEntry = m_pTextureList;

    while (pEntry != nullptr)
    {
        m_pTextureList = pEntry->m_pNext;

        delete pEntry;

        pEntry = m_pTextureList;
    }
}

// Later levels store the squared threshold bands, so the runtime never takes a square root.

// GLOBAL: HYPERX 0x004b39d8
extern const float g_flDegreesToRadians;

// FUNCTION: HYPERX 0x004728e0
LODNode* ModelDatabase::LoadLOD(char* pszModelName)
{
    LODNode* pResult;
    short    nLodCount;
    short    i;
    float    afThresholds[kLODLevelCount];

    auto* pNode = FindLOD(pszModelName);

    if (pNode != nullptr)
    {
        return pNode;
    }

    HwRenderer::ForgetMeshes();

    OwnedFile file;

    if (ModelFile::Open(&file.m_file, pszModelName) != 0)
    {
        const auto nCode = file.m_file.Read(&nLodCount, sizeof(nLodCount));

        if (nCode == 0 || file.m_file.Read(afThresholds, sizeof(afThresholds)) == 0)
        {
            g_gameApp.Error(
                // STRING: HYPERX 0x004c5364
                "Error reading Model Lod header!");

            return nullptr;
        }

        auto* pBlob = ModelComponent::ReadFirst(&file.m_file);

        if (pBlob != nullptr)
        {
            pBlob->m_nId = m_nNextIndex;

            pBlob->m_vAngle.x = g_flDegreesToRadians * pBlob->m_vAngle.x;
            pBlob->m_vAngle.y = g_flDegreesToRadians * pBlob->m_vAngle.y;
            pBlob->m_vAngle.z = g_flDegreesToRadians * pBlob->m_vAngle.z;

            auto* const pRaw = new LODNode;

            if (pRaw != nullptr)
            {
                pNode = pRaw->Init(nLodCount);
            }
            else
            {
                pNode = nullptr;
            }

            pResult = pNode;

            // Boundary i is farther[i] on the way out and nearer[i - 1] on the way back.
            for (i = 1; i < nLodCount; i++)
            {
                auto flEdge = afThresholds[i];
                flEdge += m_flLodHysteresis;
                pNode->m_aflFarther[i] = flEdge * flEdge;

                flEdge                    = afThresholds[i] - m_flLodHysteresis;
                pNode->m_aflNearer[i - 1] = flEdge * flEdge;
            }

            pNode->m_pRecord = pBlob;

            ReadMaterials(&file.m_file, pNode);
            LinkComponents(&file.m_file, pNode);

            file.m_file.CloseAndTest();

            if (m_pListHead == nullptr)
            {
                m_pListTail = pNode;
                m_pListHead = pNode;
            }
            else
            {
                m_pListTail->m_pNext = pNode;
                m_pListTail          = pNode;
            }

            m_nNextIndex++;
            m_nNodeCount++;

            pBlob           = pNode->m_pRecord;
            auto nFaceMax   = m_nFaceMax;
            auto nVertexMax = m_nVertexMax;

            for (i = 0; i < pBlob->m_nSubComponents; i++)
            {
                const MeshBlock* pSubBlob = pNode->m_apLodMeshes[0][i];

                if (pSubBlob != nullptr)
                {
                    if (nVertexMax <= pSubBlob->nVertexCount)
                    {
                        nVertexMax = pSubBlob->nVertexCount;
                    }

                    if (nFaceMax <= pSubBlob->nFaceCount)
                    {
                        nFaceMax = pSubBlob->nFaceCount;
                    }
                }
            }

            if (nVertexMax > m_nVertexMax)
            {
                delete[] g_pModelVertexBuf;

                m_nVertexMax = nVertexMax;

                g_pModelVertexBuf = new TransformedVertex[nVertexMax];

                memset(g_pModelVertexBuf, 0, m_nVertexMax * sizeof(TransformedVertex));
            }

            if (nFaceMax > m_nFaceMax)
            {
                delete[] g_ppFaceList;
                delete[] g_pFaceRecords;

                m_nFaceMax = nFaceMax;

                // The face list is one based.
                g_ppFaceList = new FaceRecord*[nFaceMax + 1];

                memset(g_ppFaceList, 0, m_nFaceMax * sizeof(FaceRecord*));

                g_pFaceRecords = new FaceRecord[m_nFaceMax];

                memset(g_pFaceRecords, 0, m_nFaceMax * sizeof(FaceRecord));
            }
        }
        else
        {
            file.m_file.CloseAndTest();

            g_gameApp.Error(
                // STRING: HYPERX 0x004c5384
                "Error initializing Model file %s!",
                pszModelName);

            pResult = nullptr;
        }
    }
    else
    {
        pResult = nullptr;
    }

    return pResult;
}

// Count, source file name, skipped names, MeshMaterial[count], then texture count and names.

// FUNCTION: HYPERX 0x00472c00
void ModelDatabase::ReadMaterials(FileObj* pFile, LODNode* pLodNode)
{
    unsigned char     nMatCount;
    char              szMatFile[16];
    MaterialListNode* pLookup;

    if (pFile->Read(&nMatCount, sizeof(nMatCount)) != 0)
    {
        if (pFile->Read(szMatFile, sizeof(szMatFile)) != 0)
        {
            if (pFile->SeekRelative(nMatCount * sizeof(MaterialName), FILE_SEEK_CURRENT) != 0)
            {
                auto* const pMaterials = new MeshMaterial[nMatCount];

                MaterialName* paTexNames = nullptr;

                // The texture field holds a pointer once loaded, so records are read in their file layout.
                MeshMaterialRecord aRecords[256] = {};

                const int nRead = pFile->Read(aRecords, nMatCount * sizeof(MeshMaterialRecord));

                for (int i = 0; i < nMatCount; i++)
                {
                    pMaterials[i].nColour       = aRecords[i].nColour;
                    pMaterials[i].flAmbient     = aRecords[i].flAmbient;
                    pMaterials[i].flDiffuse     = aRecords[i].flDiffuse;
                    pMaterials[i].flEmissive    = aRecords[i].flEmissive;
                    pMaterials[i].nTextureIndex = aRecords[i].nTextureIndex;
                    pMaterials[i].nBackIndex    = aRecords[i].nBackIndex;
                }

                if (nRead != 0)
                {
                    short nTexCount;

                    if (pFile->Read(&nTexCount, sizeof(nTexCount)) != 0)
                    {
                        if (nTexCount != 0)
                        {
                            int nTexBytes;

                            nTexBytes = nTexCount * sizeof(MaterialName);

                            paTexNames = new MaterialName[nTexCount];

                            if (pFile->Read(paTexNames[0], nTexBytes) == 0)
                            {
                                g_gameApp.Fatal(
                                    // STRING: HYPERX 0x004c5488
                                    "Error reading texture names for model %s!",
                                    pLodNode->m_pRecord->m_szName);
                            }
                        }

                        pLookup = CreateMaterialList(pMaterials, nMatCount, paTexNames, nTexCount);

                        pLodNode->m_pMaterials = pLookup;

                        pLookup->m_pszName = strcpy(new char[strlen(szMatFile) + 1], szMatFile);
                    }
                    else
                    {
                        g_gameApp.Fatal(
                            // STRING: HYPERX 0x004c545c
                            "Error reading texture count for model %s!",
                            pLodNode->m_pRecord->m_szName);
                    }
                }
                else
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c5434
                        "Error reading Materials for model %s!",
                        pLodNode->m_pRecord->m_szName);
                }
            }
            else
            {
                g_gameApp.Fatal(
                    // STRING: HYPERX 0x004c5408
                    "Error reading Material names for model %s!",
                    pLodNode->m_pRecord->m_szName);
            }
        }
        else
        {
            g_gameApp.Fatal(
                // STRING: HYPERX 0x004c53d8
                "Error reading material file name for model %s!",
                pLodNode->m_pRecord->m_szName);
        }
    }
    else
    {
        g_gameApp.Fatal(
            // STRING: HYPERX 0x004c53a8
            "Error reading Material List count for model %s!",
            pLodNode->m_pRecord->m_szName);
    }
}

// On an empty chain the original stores through null. Nothing unlinks a list.

// The multibyte compare at 0x0049f2d2, not _stricmp.

// FUNCTION: HYPERX 0x00472e10
MaterialListNode* ModelDatabase::FindOrLoadMaterialList(LODNode* pLod, char* pszName)
{
    auto* pLookup = pLod->m_pMaterials;
    auto* pPrev   = pLookup;

    if (pszName != nullptr)
    {
        while (pLookup != nullptr)
        {
            if (SDL_strcasecmp(pszName, pLookup->GetName()) == 0)
            {
                pLookup->m_nRefs++;

                return pLookup;
            }

            pPrev   = pLookup;
            pLookup = pLookup->m_pNext;
        }

        // Not in the list, so it is loaded and appended.
        MeshMaterial* pMaterials = nullptr;
        MaterialName* paMatNames = nullptr;
        MaterialName* paTexNames = nullptr;
        short         nTexCount  = 0;
        unsigned char nMatCount  = 0;

        // STRING: HYPERX 0x004c5538
        PathString path(kMaxPathLength, g_gameApp.m_strModelPath.m_str.m_pchData, pszName, "mat");
        OwnedFile  file;

        if (file.m_file.Open(path.m_str.m_pchData, FILE_MODE_READ) != 0)
        {
            if (MeshMaterial::ParseFile(&file.m_file, pszName, &pMaterials, &paMatNames, &paTexNames, &nMatCount, &nTexCount) != 0)
            {
                char* pszCopy;

                file.m_file.CloseAndTest();

                pLookup = pPrev->m_pNext =
                    CreateMaterialList(pMaterials, nMatCount, paTexNames, nTexCount);

                pszCopy            = new char[strlen(pszName) + 1];
                pLookup->m_pszName = strcpy(pszCopy, pszName);

                delete[] paMatNames;
            }
            else
            {
                file.m_file.CloseAndTest();

                g_gameApp.Fatal(
                    // STRING: HYPERX 0x004c54f4
                    "materialList File  %s in model directory could not be initialized!",
                    path.m_str.m_pchData);

                return nullptr;
            }
        }
        else
        {
            g_gameApp.Fatal(
                // STRING: HYPERX 0x004c54b4
                "MaterialList File %s in model directory could not be opened!",
                path.m_str.m_pchData);

            return nullptr;
        }
    }

    pLookup->m_nRefs++;

    return pLookup;
}

// FUNCTION: HYPERX 0x00473240
short ModelDatabase::LinkComponents(FileObj* pFile, LODNode* pLodNode)
{
    MeshBlock**    pBlobs;
    unsigned short nCount = 0;

    auto* const pBase = pLodNode->m_pRecord;

    if (pBase->m_nSubComponents != 0)
    {
        for (int nLod = 0; nLod < pLodNode->m_wLodCount; nLod++)
        {
            ModelComponent::ReadAll(pFile, &pBlobs, &nCount);

            auto** const pSlots = new MeshBlock*[pBase->m_nSubComponents];

            pLodNode->m_apLodMeshes[nLod] = pSlots;

            memset(pSlots, 0, pBase->m_nSubComponents * sizeof(MeshBlock*));

            for (short iBlob = 0; iBlob < static_cast<short>(nCount); iBlob++)
            {
                auto* const pBlob = pBlobs[iBlob];

                ComputeNormals(pBlob, pLodNode);

                pSlots[pBlob->nSlot] = pBlob;
            }

            delete[] pBlobs;
        }
    }

    return static_cast<short>(nCount);
}

// A miss takes its reference without testing the pointer, as in the original.
// FUNCTION: HYPERX 0x00473330
TexCacheEntry* ModelDatabase::FindOrLoadTexture(char* pszName)
{
    auto*          pEntry = m_pTextureList;
    TexCacheEntry* pPrev  = nullptr;

    while (pEntry != nullptr)
    {
        if (SDL_strcasecmp(pEntry->m_pszName, pszName) == 0)
        {
            pEntry->m_wRefs++;

            return pEntry;
        }

        pPrev  = pEntry;
        pEntry = pEntry->m_pNext;
    }

    auto* const pNew = new TexCacheEntry(pszName);

    if (pPrev != nullptr)
    {
        pPrev->m_pNext = pNew;
    }
    else
    {
        m_pTextureList = pNew;
    }

    pNew->m_wRefs++;

    return pNew;
}

// The cross product is the second edge crossed with the first.
// FUNCTION: HYPERX 0x004734a0
void ModelDatabase::ComputeNormals(MeshBlock* pBlob, LODNode*)
{
    const Vec3* pVerts;

    if (pBlob == nullptr || pBlob->vertexOffset.nBytes == 0)
    {
        pVerts = nullptr;
    }
    else
    {
        pVerts = pBlob->Vertices();
    }

    for (short nFace = 0; nFace < pBlob->nFaceCount; nFace++)
    {
        Vec3 v0;
        Vec3 v1;
        Vec3 v2;

        auto*       pFace   = pBlob->Face(nFace);
        auto* const pNormal = &pFace->vNormal;

        v2 = pVerts[pFace->aCorners[2]];
        v1 = pVerts[pFace->aCorners[1]];
        v0 = pVerts[pFace->aCorners[0]];

        Vec3 vE2;

        const auto flE1X = v1.x - v0.x;
        const auto flE1Y = v1.y - v0.y;
        const auto flE1Z = v1.z - v0.z;

        vE2.x = v2.x - v0.x;
        vE2.y = v2.y - v0.y;
        vE2.z = v2.z - v0.z;

        pNormal->x = vE2.z * flE1Y - vE2.y * flE1Z;
        pNormal->y = vE2.x * flE1Z - vE2.z * flE1X;
        pNormal->z = vE2.y * flE1X - vE2.x * flE1Y;

        Math::Vec3Normalize(pNormal);
    }
}

// FUNCTION: HYPERX 0x004735c0
LODNode* ModelDatabase::FindLOD(const char* pszName)
{
    auto* pNode = m_pListHead;

    while (pNode != nullptr)
    {
        if (SDL_strcasecmp(pNode->m_pRecord->m_szName, pszName) == 0)
        {
            return pNode;
        }

        pNode = pNode->m_pNext;
    }

    return nullptr;
}

// FUNCTION: HYPERX 0x00473660
MaterialListNode* ModelDatabase::CreateMaterialList(MeshMaterial* pMaterials, unsigned char nMatCount, MaterialName* paTexNames, short nTexCount)
{
    auto* const pLookup = new MaterialListNode(nMatCount);

    pLookup->m_pMaterials = pMaterials;

    for (short i = 0; i < nMatCount; i++)
    {
        const auto nTexIndex = static_cast<short>(pLookup->m_pMaterials[i].nTextureIndex);

        if (nTexIndex > 0)
        {
            char* const pszName = paTexNames[nTexIndex - 1];

            pLookup->m_pMaterials[i].pTexture = FindOrLoadTexture(pszName);

            if (pLookup->m_pMaterials[i].pTexture == nullptr)
            {
                g_gameApp.Fatal(
                    // STRING: HYPERX 0x004c55b0
                    "Error reading activating texture %s!",
                    *pszName);
            }
        }
    }

    if (nTexCount != 0)
    {
        delete[] paTexNames;
    }

    return pLookup;
}
