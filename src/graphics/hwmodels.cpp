#include "common/compat.h"
#include "graphics/bitmapformat.h"
#include "graphics/hwmodels.h"
#include "app/gameapp.h"
#include "engine/engineobject.h"
#include "engine/objectclasses.h"
#include "engine/objectdef.h"
#include "graphics/cd3dtexturemanager.h"
#include "graphics/colorpalette.h"
#include "graphics/graphicssettings.h"
#include "graphics/lodnode.h"
#include "graphics/meshbone.h"
#include "graphics/meshmaterial.h"
#include "graphics/modelcomponent.h"
#include "graphics/modeldatabase.h"
#include "globals.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace
{

struct TexturedModel
{
    LODNode*                     pLod       = nullptr;
    MaterialListNode*            pMaterials = nullptr;
    std::vector<TransformObject> world;
    unsigned int                 nPosedFrame = ~0u;
};

std::map<const EngineObject*, TexturedModel> s_models;
std::set<const MaterialListNode*>            s_converted;

// Made once and kept for the life of the process.
CD3DTextureManager& Database()
{
    static auto s_database = CD3DTextureManager();

    return s_database;
}

// Both databases size the shared transform and face buffers from their own maximum, so the two
// maxima are kept equal or one would shrink the buffers the other's models need.
void ShareBufferSizes()
{
    auto& textured = Database();

    const auto nVertexMax = (std::max)(textured.m_nVertexMax, g_pTexManager->m_nVertexMax);
    const auto nFaceMax   = (std::max)(textured.m_nFaceMax, g_pTexManager->m_nFaceMax);

    textured.m_nVertexMax       = nVertexMax;
    textured.m_nFaceMax         = nFaceMax;
    g_pTexManager->m_nVertexMax = nVertexMax;
    g_pTexManager->m_nFaceMax   = nFaceMax;
}

// The model path is global, so it names the textured set only while that set loads.
class TexturedPath
{
public:
    TexturedPath()
    : m_strSaved(g_gameApp.m_strModelPath.m_str.m_pchData)
    {
        g_gameApp.m_strModelPath.m_str.AssignFromString(String::Concat(g_gameApp.m_strExeDir.m_str, "Models\\Mdltxt"));
    }

    ~TexturedPath()
    {
        g_gameApp.m_strModelPath.m_str.AssignFromString(m_strSaved);
    }

    TexturedPath(const TexturedPath&)            = delete;
    TexturedPath& operator=(const TexturedPath&) = delete;

private:
    String m_strSaved;
};

// As EngineObject::InitModel did for Direct3D: the palette index, team remapped, becomes RGB.
void ConvertColours(MaterialListNode& materials, const EngineObject& object, const ObjectDef& def)
{
    if (s_converted.contains(&materials))
    {
        return;
    }

    s_converted.insert(&materials);

    const auto bRemap = (object.m_nSlot & 1) != 0 || def.m_bForceRemap != 0;

    for (int i = 0; i < materials.m_nMatCount; i++)
    {
        auto& material = materials.m_pMaterials[i];

        const auto nColour = material.nColour & 0xff;

        const auto nIndex = [&]
        {
            if (bRemap && nColour >= g_pColorPalette->m_remapRange.nFirst && nColour <= g_pColorPalette->m_remapRange.nCount)
            {
                return g_pColorPalette->m_teamARange.nCount + nColour;
            }

            return nColour;
        }();

        PaletteEntry entry;

        const auto& remapped = *g_pColorPalette->GetRemappedEntry(&entry, static_cast<unsigned char>(nIndex));

        material.nColour = static_cast<int>(0xff000000u | (remapped.peRed << 16) | (remapped.peGreen << 8) | remapped.peBlue);
    }
}

// The engine poses the shaded rig. The textured rig shares its hierarchy and the animation's
// rotations, but every bone below the root sits at the textured file's rest offset.
void Pose(const EngineObject& object, TexturedModel& model)
{
    const auto nBones = static_cast<int>(object.m_wBoneCount);

    model.world.resize(static_cast<size_t>(nBones));

    // RokObject::ApplyRenderSlotPose sets each bone straight in the world, keeping no local
    // matrices to rebuild the rig from.
    if (object.m_nSlot == kSlotRok)
    {
        for (int i = 0; i < nBones; i++)
        {
            model.world[static_cast<size_t>(i)] = object.m_pBones[i].m_world;
        }

        return;
    }

    auto parents = std::vector<int>(static_cast<size_t>(nBones), -1);

    for (int i = 0; i < nBones; i++)
    {
        const auto& bone = object.m_pBones[i];

        if (bone.m_apChildren == nullptr || bone.m_pDesc == nullptr)
        {
            continue;
        }

        for (int nChild = 0; nChild < bone.m_pDesc->m_nChildCount; nChild++)
        {
            const auto nIndex = static_cast<int>(bone.m_apChildren[nChild] - object.m_pBones);

            if (nIndex >= 0 && nIndex < nBones && nIndex != i)
            {
                parents[static_cast<size_t>(nIndex)] = i;
            }
        }
    }

    auto unscaled = std::vector<TransformObject>(static_cast<size_t>(nBones));
    auto posed    = std::vector<bool>(static_cast<size_t>(nBones), false);

    // Parents before children; a hierarchy is at most as deep as it has bones.
    for (int nPass = 0; nPass < nBones; nPass++)
    {
        for (int i = 0; i < nBones; i++)
        {
            const auto nParent = parents[static_cast<size_t>(i)];

            if (posed[static_cast<size_t>(i)] || (nParent >= 0 && !posed[static_cast<size_t>(nParent)]))
            {
                continue;
            }

            const auto& bone = object.m_pBones[i];

            if (nParent < 0)
            {
                unscaled[static_cast<size_t>(i)] = bone.m_world;
            }
            else
            {
                auto        local = bone.m_local;
                const auto& rest  = model.pLod->m_pRecord->SlotDesc(bone.m_wMeshSlot)->m_vRest;

                local.SetTranslation(rest.x, rest.y, rest.z);

                unscaled[static_cast<size_t>(nParent)].Multiply(&unscaled[static_cast<size_t>(i)], &local);
            }

            model.world[static_cast<size_t>(i)] = unscaled[static_cast<size_t>(i)];

            // As MeshBone::PropagateTransforms: scale after the children, so it is never inherited.
            if (bone.m_bHasScale != 0 && nParent >= 0)
            {
                model.world[static_cast<size_t>(i)].m_vScale = bone.m_world.m_vScale;
                model.world[static_cast<size_t>(i)].ApplyScale();
            }

            posed[static_cast<size_t>(i)] = true;
        }
    }
}

} // namespace

void HwModels::Load(EngineObject& object, const ObjectDef& def)
{
    Forget(object);

    if (object.HasNoModel() != 0)
    {
        return;
    }

    ShareBufferSizes();

    LODNode*          pLod       = nullptr;
    MaterialListNode* pMaterials = nullptr;

    {
        const auto path = TexturedPath();

        pLod = Database().LoadLOD(def.GetModel());

        if (pLod != nullptr)
        {
            pMaterials = Database().FindOrLoadMaterialList(pLod, def.m_pszMaterial);
        }
    }

    ShareBufferSizes();

    // Without a textured version, or with a rig that does not match, the shaded model draws.
    if (pLod == nullptr || pMaterials == nullptr || pLod->m_pRecord == nullptr || pLod->m_pRecord->m_nSubComponents != object.m_wBoneCount)
    {
        return;
    }

    (pLod->m_wRefCount)++;

    ConvertColours(*pMaterials, object, def);

    s_models[&object] = TexturedModel{ pLod, pMaterials };
}

void HwModels::Forget(const EngineObject& object)
{
    s_models.erase(&object);
}

bool HwModels::Find(MeshBone& bone, unsigned int nFrame, Bone& out)
{
    const auto it = s_models.find(bone.m_pOwner);

    if (it == s_models.end())
    {
        return false;
    }

    auto&       model  = it->second;
    const auto& object = *bone.m_pOwner;

    if (model.nPosedFrame != nFrame)
    {
        Pose(object, model);

        model.nPosedFrame = nFrame;
    }

    const auto nBone = static_cast<int>(&bone - object.m_pBones);
    const auto nLod  = [&]
    {
        if (GraphicsSettings::On(GraphicsSettings::Switch::FULL_DETAIL))
        {
            return 0;
        }

        return (std::min)(static_cast<int>(object.m_wLodLevel), model.pLod->m_wLodCount - 1);
    }();

    if (nBone < 0 || nBone >= static_cast<int>(model.world.size()) || nLod < 0)
    {
        return false;
    }

    const MeshBlock* const pBlock = model.pLod->m_apLodMeshes[nLod][bone.m_wMeshSlot].get();

    if (pBlock == nullptr)
    {
        return false;
    }

    out = Bone{ pBlock, model.pMaterials->m_pMaterials.get(), model.pMaterials->m_nMatCount, &model.world[static_cast<size_t>(nBone)] };

    return true;
}
