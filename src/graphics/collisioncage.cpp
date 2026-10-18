// Twelve faces of five vertices, twelve normals at 0x2d0 and twelve bound pairs at 0x360.

#include "common/compat.h"
#include "physics/collisioncage.h"
#include "meshbone.h"
#include "common/staticinit.h"
#include "math/vector.h"
#include "engine/devicector.h"

namespace Component
{

static inline const MeshBone* Get(const EngineObject* pObject, int nComponent);

}

static inline const MeshBone* Component::Get(const EngineObject* pObject, int nComponent)
{
    return &pObject->m_pBones[nComponent];
}

inline void CollisionCage::TransformCorner(CollisionCage* pCage, const Vec3* pVertices, const MeshFace* pFace, short nFace, short nVertex)
{
    Vec3*                  pWorldVertex;
    Vec3                   vPos;
    const TransformObject* pXf;
    short                  nVertIndex;

    nVertIndex = pFace->aCorners[nVertex];

    pWorldVertex = &pCage->aFace[nFace][nVertex];

    vPos = pVertices[nVertIndex];

    pXf = &pCage->pOwner->m_transformC;

    pWorldVertex->x = vPos.x * pXf->m_a[0][0] + vPos.y * pXf->m_a[0][1] + vPos.z * pXf->m_a[0][2] + pXf->m_a[0][3];
    pWorldVertex->y = vPos.x * pXf->m_a[1][0] + vPos.y * pXf->m_a[1][1] + vPos.z * pXf->m_a[1][2] + pXf->m_a[1][3];
    pWorldVertex->z = vPos.x * pXf->m_a[2][0] + vPos.y * pXf->m_a[2][1] + vPos.z * pXf->m_a[2][2] + pXf->m_a[2][3];
}

inline void CollisionCage::ComputeNormal(CollisionCage* pCage, short nFace)
{
    Vec3* pNormal;
    Vec3  vVert2;
    Vec3  vVert1;
    Vec3  vVert0;
    Vec3  vSide1;
    Vec3  vSide0;

    pNormal = &pCage->aNormal[nFace];

    vVert2 = pCage->aFace[nFace][2];
    vVert1 = pCage->aFace[nFace][1];
    vVert0 = pCage->aFace[nFace][0];

    vSide0.x = vVert1.x - vVert0.x;
    vSide0.y = vVert1.y - vVert0.y;
    vSide0.z = vVert1.z - vVert0.z;

    vSide1.x = vVert2.x - vVert0.x;
    vSide1.y = vVert2.y - vVert0.y;
    vSide1.z = vVert2.z - vVert0.z;

    pNormal->x = vSide0.y * vSide1.z - vSide0.z * vSide1.y;
    pNormal->y = vSide0.z * vSide1.x - vSide0.x * vSide1.z;
    pNormal->z = vSide0.x * vSide1.y - vSide0.y * vSide1.x;

    Math::Vec3Normalize(pNormal);
}

// The eight bound stores recompute the same four pairs on every face pass.

// FUNCTION: HYPERX 0x0042a280
void CollisionCage::ComputeFaceNormals()
{
    MeshFace*       pFace;
    short           nVertices;
    short           nVertex;
    const MeshBone* pComponent;
    short           nFacesToDo;
    short           nMeshFace;
    short           iClearVertex;
    short           iClearFace;

    for (iClearFace = 0; iClearFace < kCageFaces; iClearFace++)
    {
        aNormal[iClearFace].x = aNormal[iClearFace].y = aNormal[iClearFace].z = g_flZeroFaceNormal;
        aBound[iClearFace].x = aBound[iClearFace].y = aBound[iClearFace].z = g_flZeroFaceNormal;

        for (iClearVertex = 0; iClearVertex < kCageVertices; iClearVertex++)
        {
            aFace[iClearFace][iClearVertex].x = aFace[iClearFace][iClearVertex].y = aFace[iClearFace][iClearVertex].z = g_flZeroFaceNormal;
        }
    }

    switch (nRegistrationId)
    {
        default:
            return;

        case 0:
            break;
    }

    pComponent = Component::Get(pOwner, 0);

    nFacesToDo = pComponent->GetFaceCount() <= kCageFaces ? pComponent->GetFaceCount() : static_cast<short>(kCageFaces);

    for (nMeshFace = 0; nMeshFace < nFacesToDo; nMeshFace++)
    {
        pFace = pComponent->GetFace(nMeshFace);

        nVertices = pFace->nCorners <= kCageVertices ? pFace->nCorners : static_cast<short>(kCageVertices);

        for (nVertex = 0; nVertex < nVertices; nVertex++)
        {
            TransformCorner(this, pComponent->GetVertexArray(), pFace, nMeshFace, nVertex);
        }

        aBound[0].x = aFace[2][0].x - pOwner->GetPositionPtr()->x;

        aBound[0].y = aFace[2][0].y - pOwner->GetPositionPtr()->y;

        aBound[1].x = aFace[3][2].x - pOwner->GetPositionPtr()->x;

        aBound[1].y = aFace[3][2].y - pOwner->GetPositionPtr()->y;

        aBound[2].x = aFace[3][0].x - pOwner->GetPositionPtr()->x;

        aBound[2].y = aFace[3][0].y - pOwner->GetPositionPtr()->y;

        aBound[3].x = aFace[2][2].x - pOwner->GetPositionPtr()->x;

        aBound[3].y = aFace[2][2].y - pOwner->GetPositionPtr()->y;

        ComputeNormal(this, nMeshFace);
    }
}
