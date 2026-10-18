#include "common/compat.h"
#include "graphics/face.h"
#include "globals.h"
#include "cd3dtexturemanager.h"
#include "common/pathstring.h"
#include "texcacheentry.h"
#include "lodnode.h"
#include "modeldatabase.h"
#include "meshmaterial.h"
#include "meshbone.h"
#include "meshblock.h"
#include "graphics/texcoord.h"
#include "ui/widget.h"
#include "app/gameapp.h"
#include <new>
#include <cstdlib>
#include <cstring>
#include "engine/object.h"
#include "graphics/softtex.h"
#include "modelcomponent.h"

// Frees the node only when its m_wRefCount drops to zero.
// FUNCTION: HYPERX 0x00473020
char CD3DTextureManager::ReleaseLOD(LODNode* pLod, MaterialListNode* pBlock)
{
    LODNode* pAt;

    if (pLod == nullptr)
    {
        return 0;
    }

    if (DeactivateMaterialLookup(pLod, pBlock) != 0)
    {
        pLod->m_wRefCount -= 1;

        if (pLod->m_wRefCount != 0)
        {
            return 1;
        }

        LODNode* pPrev = nullptr;

        for (pAt = m_pListHead; pAt != nullptr; pAt = pAt->m_pNext)
        {
            if (pLod == pAt)
            {
                if (m_pListTail == pAt)
                {
                    m_pListTail = pPrev;
                }

                if (pPrev != nullptr)
                {
                    pPrev->m_pNext = pAt->m_pNext;
                }
                else
                {
                    pAt = pAt->m_pNext;

                    m_pListHead = pAt;

                    if (pAt == nullptr)
                    {
                        m_pListTail = pAt;
                    }
                }

                ReleaseModel(pLod);

                delete pLod;

                return 1;
            }

            pPrev = pAt;
        }

        return 0;
    }

    // STRING: HYPERX 0x004c553c
    g_gameApp.Fatal("Error deactivating texture for model %s!",
                    pLod->m_pRecord->m_szName);

    return 0;
}

// The head is re-read after every step, because releasing an entry can run arbitrary code.

// FUNCTION: HYPERX 0x004730d0
char CD3DTextureManager::ReleaseModel(LODNode* pLod)
{
    short iRecord;

    while (pLod->m_pMaterials != nullptr)
    {
        for (iRecord = 0; iRecord < pLod->m_pMaterials->m_nMatCount; iRecord++)
        {
            TexCacheEntry* const pEntry = pLod->m_pMaterials->m_pMaterials[iRecord].pTexture;

            if (pEntry != nullptr)
            {
                ReleaseTexEntry(pEntry);
            }
        }

        MaterialListNode* const pList = pLod->m_pMaterials;

        pLod->m_pMaterials = pList->m_pNext;

        if (pList != nullptr)
        {
            MeshMaterial* const pArray = pList->m_pMaterials;

            delete[] pArray;
            delete[] pList->m_pszName;
            delete pList;
        }
    }

    return 1;
}

// Frees the block only when its m_nRefs drops to zero.
// FUNCTION: HYPERX 0x00473180
char CD3DTextureManager::DeactivateMaterialLookup(LODNode* pLod, MaterialListNode* pBlock)
{
    MaterialListNode* pAt;

    MaterialListNode** ppLink = &pLod->m_pMaterials;

    char bReleased = 0;

    for (; (pAt = *ppLink) != nullptr; ppLink = &pAt->m_pNext)
    {
        if (pBlock == pAt)
        {
            pBlock->m_nRefs -= 1;

            if (pBlock->m_nRefs == 0)
            {
                for (short iRecord = 0; iRecord < pBlock->m_nMatCount; iRecord++)
                {
                    TexCacheEntry* const pEntry = pBlock->m_pMaterials[iRecord].pTexture;

                    if (pEntry != nullptr)
                    {
                        ReleaseTexEntry(pEntry);
                    }
                }

                *ppLink = pBlock->m_pNext;

                if (pBlock != nullptr)
                {
                    MeshMaterial* const pArray = pBlock->m_pMaterials;

                    delete[] pArray;
                    delete[] pBlock->m_pszName;
                    delete pBlock;
                }
            }

            bReleased = 1;

            break;
        }
    }

    return bReleased;
}

// FUNCTION: HYPERX 0x004733d0
char CD3DTextureManager::ReleaseTexEntry(TexCacheEntry* pEntry)
{
    if (pEntry != nullptr)
    {
        pEntry->m_wRefs -= 1;

        if (pEntry->m_wRefs == 0)
        {
            FreeTexEntry(pEntry);
        }

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00473400
char CD3DTextureManager::FreeTexEntry(TexCacheEntry* pEntry)
{
    TexCacheEntry* pAt   = m_pTextureList;
    TexCacheEntry* pPrev = pAt;

    while (pAt != nullptr)
    {
        if (pAt == pEntry)
        {
            if (pAt->m_wRefs == 0)
            {
                if (pAt == m_pTextureList)
                {
                    m_pTextureList = pAt->m_pNext;
                }
                else
                {
                    pPrev->m_pNext = pAt->m_pNext;
                }

                pAt->DestroyTexObj();

                if (pAt == m_pTextureList)
                {
                    m_pTextureList = nullptr;
                }

                delete pAt;

                return 1;
            }

            // STRING: HYPERX 0x004c5568
            g_gameApp.Error("Can't deactivate active texture item!");

            return 0;
        }

        pPrev = pAt;
        pAt   = pAt->m_pNext;
    }

    // STRING: HYPERX 0x004c5590
    g_gameApp.Error("DeActivating inactive texture!");

    return 0;
}

// SlotDesc turns the slot into a descriptor whose link id is the mesh index.
// FUNCTION: HYPERX 0x00473600
int CD3DTextureManager::LookupMeshForSlot(LODNode* pNode, short nLevel, short nSlot, MeshBlock** ppOut)
{
    MeshBlock* pMesh;

    if (pNode->m_pRecord != nullptr)
    {
        const short nIndex = pNode->m_pRecord->SlotDesc(nSlot)->m_nLinkId;

        if (nIndex < pNode->m_pRecord->m_nSubComponents)
        {
            pMesh = pNode->m_apLodMeshes[nLevel][nIndex];
        }
        else
        {
            pMesh = nullptr;
        }

        *ppOut = pMesh;

        return pMesh != nullptr;
    }

    return -1;
}

// uv *= (width - 1) / the width last scaled to, clamped with negatives wrapped. Software renderer only.
// FUNCTION: HYPERX 0x00473720
int CD3DTextureManager::RescaleSoftTexCoords(short nMeshIndex, LODNode* pLod, MaterialListNode* pList)
{
    short*        pnCoord;
    MeshMaterial* pEntry;
    short         nScaledWidth;

    short nLevel = 0;

    while (nLevel < pLod->m_wLodCount)
    {
        const MeshBlock* pMesh = pLod->m_apLodMeshes[nLevel][nMeshIndex];

        if (pMesh->nCoordBytes != 0)
        {
            TexCoord* pCoords = pMesh->Coords();

            char* const pScratch = new char[pMesh->nCoordBytes];

            memset(pScratch, 0, pMesh->nCoordBytes);

            for (short nFace = 0; nFace < pMesh->nFaceCount; nFace++)
            {
                MeshFace* const pFace = pMesh->Face(nFace);

                if ((pFace->cFlags & FACE_FLAG_HAS_UV) != 0)
                {
                    pnCoord = pFace->UVIndices();
                }
                else
                {
                    pnCoord = nullptr;
                }

                while (pList != nullptr)
                {
                    pEntry = pList->m_pMaterials + pFace->cMaterial;

                    if (pEntry->pTexture != nullptr && pnCoord != nullptr)
                    {
                        SoftTex* const pSoftTex = pEntry->pTexture->m_pSoftTexture;

                        if (pSoftTex != nullptr)
                        {
                            nScaledWidth = pFace->GetScaledWidth();

                            unsigned int nWide = pSoftTex->GetWidth();

                            if (nWide - 1 != static_cast<unsigned int>(nScaledWidth))
                            {
                                nWide = pSoftTex->GetWidth();

                                const int nWideLess = nWide - 1;

                                const unsigned int nHigh = pSoftTex->GetHeight();

                                const int nHighLess = nHigh - 1;

                                nScaledWidth = pFace->GetScaledWidth();

                                const auto flWide = static_cast<float>(nWideLess);

                                const float flScale = flWide / static_cast<float>(nScaledWidth);

                                pFace->SetScaledWidth(static_cast<short>(nWideLess));

                                for (int nAt = 0; nAt < pFace->nCorners; nAt++, pnCoord++)
                                {
                                    const int nCoord = static_cast<int>(*pnCoord);

                                    if (pScratch[nCoord] == 0)
                                    {
                                        pCoords[nCoord].tu = flScale * pCoords[nCoord].tu;

                                        float* pflV = &pCoords[nCoord].tv;

                                        *pflV = flScale * *pflV;

                                        if (pCoords[nCoord].tu < 0.0f)
                                        {
                                            pCoords[nCoord].tu = flWide + pCoords[nCoord].tu;
                                        }
                                        else if (pCoords[nCoord].tu > flWide)
                                        {
                                            pCoords[nCoord].tu = flWide;
                                        }

                                        const auto flHigh = static_cast<float>(nHighLess);

                                        if (*pflV < 0.0f)
                                        {
                                            *pflV = flHigh + *pflV;
                                        }
                                        else if (flHigh < *pflV)
                                        {
                                            *pflV = flHigh;
                                        }

                                        pScratch[nCoord] = 1;
                                    }
                                }
                            }
                        }

                        break;
                    }

                    pList = pList->m_pNext;
                }
            }

            delete[] pScratch;
        }

        nLevel++;
    }

    return 1;
}
