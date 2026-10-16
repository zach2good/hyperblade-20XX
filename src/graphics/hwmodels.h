// The textured models the hardware renderer draws in place of an object's shaded ones. They load
// into their own database, from Models\Mdltxt, beside the shaded models the engine runs on, and
// each frame their bones are posed from the engine's animation with the textured rig's offsets.

#ifndef HYPERBLADE_HWMODELS_H
#define HYPERBLADE_HWMODELS_H

class EngineObject;
class MeshBone;
class TransformObject;
struct MeshBlock;
struct MeshMaterial;
class ObjectDef;

namespace HwModels
{

// Loads the textured version of the object's model once the object has loaded its own.
void Load(EngineObject& object, const ObjectDef& def);

// The object is going, so its textured model is let go too.
void Forget(const EngineObject& object);

struct Bone
{
    const MeshBlock*       pBlock;
    const MeshMaterial*    pMaterials;
    int                    nMaterialCount;
    const TransformObject* pWorld;
};

// The textured mesh, materials and world matrix for the bone this frame; false when the object
// has no textured model, which leaves the shaded one to draw.
bool Find(MeshBone& bone, unsigned int nFrame, Bone& out);

} // namespace HwModels

#endif
