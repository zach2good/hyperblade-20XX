// Twelve faces of five vertices, a normal per face, four bounds and the owner; 0x3f8 bytes.

#ifndef HYPERBLADE_COLLISIONCAGE_H
#define HYPERBLADE_COLLISIONCAGE_H

#include "math/vector.h"

#define CAGE_FACES    12
#define CAGE_VERTICES 5

class EngineObject;

struct CollisionCage
{
    ~CollisionCage() = default;

    void ComputeFaceNormals();

    CollisionCage(class EngineObject* pOwnerIn, int nId)
    {
        pOwner          = pOwnerIn;
        nRegistrationId = nId;
    }

    Vec3                aFace[CAGE_FACES][CAGE_VERTICES]; // 0x000
    Vec3                aNormal[CAGE_FACES];              // 0x2d0
    Vec3                aBound[CAGE_FACES];               // 0x360, four written
    int                 nRegistrationId;                  // 0x3f0
    class EngineObject* pOwner;                           // 0x3f4

    static void TransformCorner(CollisionCage* pCage, const Vec3* pVertices, const struct MeshFace* pFace, short nFace, short nVertex);
    static void ComputeNormal(CollisionCage* pCage, short nFace);
};

#endif
