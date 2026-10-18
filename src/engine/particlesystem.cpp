#include "common/compat.h"
#include "particle.h"
#include "globals.h"
#include "math/vector.h"
#include "graphics/meshbone.h"
#include <cmath>
#include "physics/physicsbody.h"
#include "common/staticinit.h"
#include "particlesystem.h"
#include "objectclasses.h"
#include "devicector.h"
#include "ui/hudglobals.h"
#include "engineobject.h"
#include "common/pooledclasses.h"

// How many particles a spawn fills.
constexpr auto kSpawnCount = 100;

// The ceiling on both launch parameters.
#define SPAWN_LIMIT 150.0f

// Copies five sets of twenty faces off the bone's blob into the hundred triangles.
// FUNCTION: HYPERX 0x00474d30
ParticleSystem::ParticleSystem(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    const MeshBone* pBones = m_pBones;

    m_bAnyAlive = 0;

    for (int nGroup = 0; nGroup < kParticleRowSets; nGroup++)
    {
        for (int nFace = 0; nFace < kParticleRowsPerSet; nFace++)
        {
            const MeshFace* const pRec = pBones->m_pMesh->Face(static_cast<short>(nGroup * kParticleRowsPerSet + nFace));

            for (int nCorner = 0; nCorner < 3; nCorner++)
            {
                const Vec3*            pVerts;
                const MeshBlock* const pCorner = pBones->m_pMesh;

                if (pCorner == nullptr || pCorner->vertexOffset.nBytes == 0)
                {
                    pVerts = nullptr;
                }
                else
                {
                    pVerts = pCorner->Vertices();
                }

                m_aRows[nGroup * kParticleRowsPerSet + nFace].aCorners[nCorner] =
                    pVerts[pRec->aCorners[static_cast<short>(nCorner)]];
            }
        }
    }
}

// The first of two spawners; they differ only in the initialiser they call.
// The count is re-read each pass because the initialiser can reach it.
// FUNCTION: HYPERX 0x00474ea0
void ParticleSystem::SpawnParticles(float flSpread, float flRise)
{
    Particle* pEmitter;
    int       nIndex;

    if (flSpread >= SPAWN_LIMIT)
    {
        flSpread = SPAWN_LIMIT;
    }

    if (flRise >= SPAWN_LIMIT)
    {
        flRise = SPAWN_LIMIT;
    }

    m_nParticleCount = kSpawnCount;

    for (nIndex = 0, pEmitter = m_aParticles; nIndex < m_nParticleCount; pEmitter++)
    {
        pEmitter->InitRandom(flSpread, flRise);
        nIndex++;
    }

    m_nFlags &= ~kEngineObjectHidden;

    m_bAnyAlive = 1;
}

// FUNCTION: HYPERX 0x00474f30
void ParticleSystem::SpawnParticlesUniform(float flSpread, float flRise)
{
    Particle* pEmitter;
    int       nIndex;

    if (flSpread >= SPAWN_LIMIT)
    {
        flSpread = SPAWN_LIMIT;
    }

    if (flRise >= SPAWN_LIMIT)
    {
        flRise = SPAWN_LIMIT;
    }

    m_nParticleCount = kSpawnCount;

    for (nIndex = 0, pEmitter = m_aParticles; nIndex < m_nParticleCount; pEmitter++)
    {
        pEmitter->InitRandomUniform(flSpread, flRise);
        nIndex++;
    }

    m_nFlags &= ~kEngineObjectHidden;

    m_bAnyAlive = 1;
}

#define GRAVITY -32.0f

// The alive flag stays set once any particle in the pass had life left.
// FUNCTION: HYPERX 0x00474fc0
void ParticleSystem::Update(float flDt)
{
    Vec3 vScaled;

    const float* pAxis = m_transformC.m_a[2];

    vScaled.x = pAxis[0] * GRAVITY;
    vScaled.y = pAxis[1] * GRAVITY;
    vScaled.z = pAxis[2] * GRAVITY;

    auto vAccel = vScaled;

    m_bAnyAlive = 0;

    for (int nIndex = 0; nIndex < m_nParticleCount; nIndex++)
    {
        if (m_aParticles[nIndex].m_flLife > 0.0f)
        {
            m_aParticles[nIndex].IntegrateStep(&vAccel, flDt);
        }

        m_bAnyAlive = m_bAnyAlive != 0 || m_aParticles[nIndex].m_flLife > 0.0f;
    }

    ApplyToMesh();
}

// Moves three mesh vertices per live particle along its axes.
// FUNCTION: HYPERX 0x004750a0
void ParticleSystem::ApplyToMesh()
{
    Vec3* pVerts;
    Vec3  vOut;
    int   nCorner;

    const MeshBone* pMesh = m_pBones;

    Particle* pEmitter = m_aParticles;

    for (int nIndex = 0; nIndex < m_nParticleCount; nIndex++, pEmitter++)
    {
        if (pEmitter->m_flLife > 0.0f)
        {
            auto vBase = pEmitter->m_vPosition;

            const auto flRatio = pEmitter->m_flLife / pEmitter->m_flSpan;

            const MeshFace* pPart = pMesh->GetFace(static_cast<short>(nIndex));

            const Vec3* pRow = m_aRows[static_cast<int>(pEmitter->m_flRow) + pEmitter->m_nSet * kParticleRowsPerSet].aCorners;

            for (nCorner = 0; nCorner < 3; nCorner++)
            {
                pVerts = pMesh->GetVertexArray();

                const auto vStep = pRow[nCorner].Scaled(flRatio);

                vOut.x = vBase.x + vStep.x;
                vOut.y = vBase.GetY() + vStep.y;
                vOut.z = vBase.GetZ() + vStep.z;

                pVerts[pPart->aCorners[static_cast<short>(nCorner)]] = vOut;
            }
        }
    }
}

// FUNCTION: HYPERX 0x004751d0
void ParticleSystem::SetOrientation(const Vec3* pPosition, const Vec3* pDirection)
{
    Vec3 vDirection;
    Vec3 vBasis;
    Vec3 vCross;

    auto* pBody = m_pBody;

    pBody->pos = *pPosition;

    GetLocalTransformPtr()->ResetToIdentity();

    GetLocalTransformPtr()->SetTranslation(g_flZeroRokObject, g_flZeroRokObject, g_flZeroRokObject);

    ApplyRenderSlotPose();

    if (!pDirection->NormalizeTo(&vDirection))
    {
        vDirection.x = vDirection.y = vDirection.z = 1.0f;
    }

    auto flAxis = 0.0f;

    if (fabs(static_cast<double>(1.0f - vDirection.x)) < 0.001f && fabs(static_cast<double>(-vDirection.y)) < 0.001f && fabs(static_cast<double>(-vDirection.z)) < 0.001f)
    {
        flAxis = 1.0f;
    }

    vBasis.x = flAxis * vDirection.z;
    vBasis.y = -vDirection.z;
    vBasis.z = vDirection.y - flAxis * vDirection.x;

    Math::Vec3Normalize(&vBasis);

    auto* const pTransform = GetLocalTransformPtr();

    if (vBasis.y != g_flZeroRokObject)
    {
        pTransform->m_a[0][2] = vDirection.x;
        pTransform->m_a[1][2] = vDirection.y;
        pTransform->m_a[2][2] = vDirection.z;

        pTransform->m_a[0][1] = vBasis.x;
        pTransform->m_a[1][1] = vBasis.y;
        pTransform->m_a[2][1] = vBasis.z;

        vCross.x = vBasis.y * vDirection.z - vBasis.z * vDirection.y;
        vCross.y = vBasis.z * vDirection.x - vBasis.x * vDirection.z;
        vCross.z = vBasis.x * vDirection.y - vBasis.y * vDirection.x;

        Math::Vec3Normalize(&vCross);

        pTransform->m_a[0][0] = vCross.x;
        pTransform->m_a[1][0] = vCross.y;
        pTransform->m_a[2][0] = vCross.z;
    }

    pBody = m_pBody;

    pBody->prevPos = pBody->pos;

    pBody->pos = *pPosition;

    ApplyRenderSlotPose();

    m_bAnyAlive = 1;
}

// FUNCTION: HYPERX 0x004753c0
void ParticleSystem::Deactivate()
{
    m_bAnyAlive = 0;
    m_nFlags |= kEngineObjectHidden;
}
