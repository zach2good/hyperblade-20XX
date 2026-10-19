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
#include <memory>
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

        for (pAt = m_pListHead.get(); pAt != nullptr; pAt = pAt->m_pNext.get())
        {
            if (pLod == pAt)
            {
                if (m_pListTail == pAt)
                {
                    m_pListTail = pPrev;
                }

                ReleaseModel(pLod);

                // Unlinking the node destroys it.
                if (pPrev != nullptr)
                {
                    pPrev->m_pNext = std::move(pAt->m_pNext);
                }
                else
                {
                    m_pListHead = std::move(pAt->m_pNext);
                }

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
            auto* const pEntry = pLod->m_pMaterials->m_pMaterials[iRecord].pTexture;

            if (pEntry != nullptr)
            {
                ReleaseTexEntry(pEntry);
            }
        }

        pLod->m_pMaterials = std::move(pLod->m_pMaterials->m_pNext);
    }

    return 1;
}

// Frees the block only when its m_nRefs drops to zero.
// FUNCTION: HYPERX 0x00473180
char CD3DTextureManager::DeactivateMaterialLookup(LODNode* pLod, MaterialListNode* pBlock)
{
    MaterialListNode* pAt;

    auto* ppLink = &pLod->m_pMaterials;

    char bReleased = 0;

    for (; (pAt = ppLink->get()) != nullptr; ppLink = &pAt->m_pNext)
    {
        if (pBlock == pAt)
        {
            pBlock->m_nRefs -= 1;

            if (pBlock->m_nRefs == 0)
            {
                for (short iRecord = 0; iRecord < pBlock->m_nMatCount; iRecord++)
                {
                    auto* const pEntry = pBlock->m_pMaterials[iRecord].pTexture;

                    if (pEntry != nullptr)
                    {
                        ReleaseTexEntry(pEntry);
                    }
                }

                // Unlinking the block destroys it.
                *ppLink = std::move(pBlock->m_pNext);
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
    auto* pAt   = m_pTextureList.get();
    auto* pPrev = pAt;

    while (pAt != nullptr)
    {
        if (pAt == pEntry)
        {
            if (pAt->m_wRefs == 0)
            {
                pAt->DestroyTexObj();

                // Unlinking the entry destroys it.
                if (pAt == m_pTextureList.get())
                {
                    m_pTextureList = std::move(pAt->m_pNext);
                }
                else
                {
                    pPrev->m_pNext = std::move(pAt->m_pNext);
                }

                return 1;
            }

            // STRING: HYPERX 0x004c5568
            g_gameApp.Error("Can't deactivate active texture item!");

            return 0;
        }

        pPrev = pAt;
        pAt   = pAt->m_pNext.get();
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
        const auto nIndex = pNode->m_pRecord->SlotDesc(nSlot)->m_nLinkId;

        if (nIndex < pNode->m_pRecord->m_nSubComponents)
        {
            pMesh = pNode->m_apLodMeshes[nLevel][nIndex].get();
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
        const MeshBlock* const pMesh = pLod->m_apLodMeshes[nLevel][nMeshIndex].get();

        if (pMesh->nCoordBytes != 0)
        {
            auto* const pCoords = pMesh->Coords();

            const auto pScratch = std::make_unique<char[]>(pMesh->nCoordBytes);

            for (short nFace = 0; nFace < pMesh->nFaceCount; nFace++)
            {
                auto* const pFace = pMesh->Face(nFace);

                if ((pFace->cFlags & kFaceFlagHasUV) != 0)
                {
                    pnCoord = pFace->UVIndices();
                }
                else
                {
                    pnCoord = nullptr;
                }

                while (pList != nullptr)
                {
                    pEntry = pList->m_pMaterials.get() + pFace->cMaterial;

                    if (pEntry->pTexture != nullptr && pnCoord != nullptr)
                    {
                        auto* const pSoftTex = pEntry->pTexture->m_pSoftTexture.get();

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

                                const auto flScale = flWide / static_cast<float>(nScaledWidth);

                                pFace->SetScaledWidth(static_cast<short>(nWideLess));

                                for (int nAt = 0; nAt < pFace->nCorners; nAt++, pnCoord++)
                                {
                                    const auto nCoord = static_cast<int>(*pnCoord);

                                    if (pScratch[nCoord] == 0)
                                    {
                                        pCoords[nCoord].tu = flScale * pCoords[nCoord].tu;

                                        auto* const pflV = &pCoords[nCoord].tv;

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

                    pList = pList->m_pNext.get();
                }
            }
        }

        nLevel++;
    }

    return 1;
}
