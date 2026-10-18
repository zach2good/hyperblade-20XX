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
#include <memory>
#include <cstring>
#include "app/gameapp.h"
#include "graphics/modelfile.h"
#include "graphics/meshbone.h"
#include <SDL3/SDL.h>

namespace
{

// The buffers every model shares, which the globals of the same names view.
auto s_pModelVertices = std::unique_ptr<TransformedVertex[]>();
auto s_pFaceList      = std::unique_ptr<FaceRecord*[]>();
auto s_pFaceRecords   = std::unique_ptr<FaceRecord[]>();

} // namespace

ModelDatabase::ModelDatabase()
: m_nInstallLevel(0)
, m_pListTail(nullptr)
, m_nNodeCount(0)
, m_nFaceMax(0)
, m_nVertexMax(0)
, m_nNextIndex(0)
, m_wUnusedA(0)
, m_wUnusedB(0)
, m_flLodHysteresis(5.0f)
{
}

// Frees every node whatever its reference count.
// FUNCTION: HYPERX 0x00472830
ModelDatabase::~ModelDatabase()
{
    m_pListHead.reset();
    m_pListTail = nullptr;

    s_pModelVertices.reset();
    g_pModelVertexBuf = nullptr;

    s_pFaceList.reset();
    g_ppFaceList = nullptr;

    s_pFaceRecords.reset();
    g_pFaceRecords = nullptr;

    m_pTextureList.reset();
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

        auto pBlob = ModelComponent::ReadFirst(&file.m_file);

        if (pBlob != nullptr)
        {
            pBlob->m_nId = m_nNextIndex;

            pBlob->m_vAngle.x = g_flDegreesToRadians * pBlob->m_vAngle.x;
            pBlob->m_vAngle.y = g_flDegreesToRadians * pBlob->m_vAngle.y;
            pBlob->m_vAngle.z = g_flDegreesToRadians * pBlob->m_vAngle.z;

            auto pOwned = std::make_unique<LODNode>();

            pNode = pOwned->Init(nLodCount);

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

            pNode->m_pRecord = std::move(pBlob);

            ReadMaterials(&file.m_file, pNode);
            LinkComponents(&file.m_file, pNode);

            file.m_file.CloseAndTest();

            if (m_pListHead == nullptr)
            {
                m_pListHead = std::move(pOwned);
            }
            else
            {
                m_pListTail->m_pNext = std::move(pOwned);
            }

            m_pListTail = pNode;

            m_nNextIndex++;
            m_nNodeCount++;

            const auto* const pRecord    = pNode->m_pRecord.get();
            auto              nFaceMax   = m_nFaceMax;
            auto              nVertexMax = m_nVertexMax;

            for (i = 0; i < pRecord->m_nSubComponents; i++)
            {
                const MeshBlock* pSubBlob = pNode->m_apLodMeshes[0][i].get();

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
                m_nVertexMax = nVertexMax;

                s_pModelVertices  = std::make_unique<TransformedVertex[]>(nVertexMax);
                g_pModelVertexBuf = s_pModelVertices.get();
            }

            if (nFaceMax > m_nFaceMax)
            {
                m_nFaceMax = nFaceMax;

                // The face list is one based.
                s_pFaceList  = std::make_unique<FaceRecord*[]>(nFaceMax + 1);
                g_ppFaceList = s_pFaceList.get();

                s_pFaceRecords = std::make_unique<FaceRecord[]>(m_nFaceMax);
                g_pFaceRecords = s_pFaceRecords.get();
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
    unsigned char nMatCount;
    char          szMatFile[16];

    if (pFile->Read(&nMatCount, sizeof(nMatCount)) != 0)
    {
        if (pFile->Read(szMatFile, sizeof(szMatFile)) != 0)
        {
            if (pFile->SeekRelative(nMatCount * sizeof(MaterialName), FileSeekOrigin::kCurrent) != 0)
            {
                auto pMaterials = std::make_unique<MeshMaterial[]>(nMatCount);

                auto paTexNames = std::unique_ptr<MaterialName[]>();

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

                            paTexNames = std::make_unique<MaterialName[]>(nTexCount);

                            if (pFile->Read(paTexNames[0], nTexBytes) == 0)
                            {
                                g_gameApp.Fatal(
                                    // STRING: HYPERX 0x004c5488
                                    "Error reading texture names for model %s!",
                                    pLodNode->m_pRecord->m_szName);
                            }
                        }

                        pLodNode->m_pMaterials = CreateMaterialList(std::move(pMaterials), nMatCount, std::move(paTexNames));

                        pLodNode->m_pMaterials->m_strName = szMatFile;
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
    auto* pLookup = pLod->m_pMaterials.get();
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
            pLookup = pLookup->m_pNext.get();
        }

        // Not in the list, so it is loaded and appended.
        auto          pMaterials = std::unique_ptr<MeshMaterial[]>();
        auto          paMatNames = std::unique_ptr<MaterialName[]>();
        auto          paTexNames = std::unique_ptr<MaterialName[]>();
        short         nTexCount  = 0;
        unsigned char nMatCount  = 0;

        // STRING: HYPERX 0x004c5538
        PathString path(kMaxPathLength, g_gameApp.m_strModelPath.m_str.m_pchData, pszName, "mat");
        OwnedFile  file;

        if (file.m_file.Open(path.m_str.m_pchData, FileMode::kRead) != 0)
        {
            if (MeshMaterial::ParseFile(&file.m_file, pszName, &pMaterials, &paMatNames, &paTexNames, &nMatCount, &nTexCount) != 0)
            {
                file.m_file.CloseAndTest();

                pPrev->m_pNext = CreateMaterialList(std::move(pMaterials), nMatCount, std::move(paTexNames));
                pLookup        = pPrev->m_pNext.get();

                pLookup->m_strName = pszName;
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
    unsigned short nCount = 0;

    const auto* const pBase = pLodNode->m_pRecord.get();

    if (pBase->m_nSubComponents != 0)
    {
        bool abWhole[kLODLevelCount] = {};

        for (int nLod = 0; nLod < pLodNode->m_wLodCount; nLod++)
        {
            // A failed header read leaves the list empty.
            auto blobs = std::vector<Record<MeshBlock> >();

            abWhole[nLod] = ModelComponent::ReadAll(pFile, &blobs);
            nCount        = static_cast<unsigned short>(blobs.size());

            pLodNode->m_apLodMeshes[nLod] = std::make_unique<Record<MeshBlock>[]>(pBase->m_nSubComponents);

            auto* const pSlots = pLodNode->m_apLodMeshes[nLod].get();

            // A block whose slot is out of range or taken is dropped with the list.
            for (auto& pBlob : blobs)
            {
                if (pBlob->nSlot < 0 || pBlob->nSlot >= pBase->m_nSubComponents || pSlots[pBlob->nSlot] != nullptr)
                {
                    abWhole[nLod] = false;

                    continue;
                }

                ComputeNormals(pBlob.get(), pLodNode);

                const auto nSlot = pBlob->nSlot;

                pSlots[nSlot] = std::move(pBlob);
            }
        }

        // A damaged level borrows each piece it lost from the nearest level that has it.
        for (int nLod = 0; nLod < pLodNode->m_wLodCount; nLod++)
        {
            if (abWhole[nLod])
            {
                continue;
            }

            for (short iSlot = 0; iSlot < pBase->m_nSubComponents; iSlot++)
            {
                if (pLodNode->m_apLodMeshes[nLod][iSlot] != nullptr)
                {
                    continue;
                }

                const MeshBlock* pDonor = nullptr;

                for (int nStep = 1; nStep < pLodNode->m_wLodCount && pDonor == nullptr; nStep++)
                {
                    if (nLod + nStep < pLodNode->m_wLodCount)
                    {
                        pDonor = pLodNode->m_apLodMeshes[nLod + nStep][iSlot].get();
                    }

                    if (pDonor == nullptr && nLod >= nStep)
                    {
                        pDonor = pLodNode->m_apLodMeshes[nLod - nStep][iSlot].get();
                    }
                }

                if (pDonor != nullptr)
                {
                    auto pCopy = MakeRecord<MeshBlock>(pDonor->nSize);

                    memcpy(pCopy.get(), pDonor, pDonor->nSize);

                    pLodNode->m_apLodMeshes[nLod][iSlot] = std::move(pCopy);
                }
            }
        }
    }

    return static_cast<short>(nCount);
}

// A miss takes its reference without testing the pointer, as in the original.
// FUNCTION: HYPERX 0x00473330
TexCacheEntry* ModelDatabase::FindOrLoadTexture(char* pszName)
{
    auto*          pEntry = m_pTextureList.get();
    TexCacheEntry* pPrev  = nullptr;

    while (pEntry != nullptr)
    {
        if (SDL_strcasecmp(pEntry->m_strName.c_str(), pszName) == 0)
        {
            pEntry->m_wRefs++;

            return pEntry;
        }

        pPrev  = pEntry;
        pEntry = pEntry->m_pNext.get();
    }

    auto        pOwned = std::make_unique<TexCacheEntry>(pszName);
    auto* const pNew   = pOwned.get();

    if (pPrev != nullptr)
    {
        pPrev->m_pNext = std::move(pOwned);
    }
    else
    {
        m_pTextureList = std::move(pOwned);
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

        auto* const pFace   = pBlob->Face(nFace);
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
    auto* pNode = m_pListHead.get();

    while (pNode != nullptr)
    {
        if (SDL_strcasecmp(pNode->m_pRecord->m_szName, pszName) == 0)
        {
            return pNode;
        }

        pNode = pNode->m_pNext.get();
    }

    return nullptr;
}

// FUNCTION: HYPERX 0x00473660
std::unique_ptr<MaterialListNode> ModelDatabase::CreateMaterialList(std::unique_ptr<MeshMaterial[]> pMaterials, unsigned char nMatCount, std::unique_ptr<MaterialName[]> paTexNames)
{
    auto pLookup = std::make_unique<MaterialListNode>(nMatCount);

    pLookup->m_pMaterials = std::move(pMaterials);

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

    return pLookup;
}

MaterialListNode::MaterialListNode(unsigned char nMatCount)
: m_nMatCount(nMatCount)
, m_nRefs(0)
{
}

// Unlinks the rest of the list one node at a time, so a long list cannot exhaust the stack.
MaterialListNode::~MaterialListNode()
{
    auto pNext = std::move(m_pNext);

    while (pNext != nullptr)
    {
        pNext = std::move(pNext->m_pNext);
    }
}
