#ifndef HYPERBLADE_PARTICLESYSTEM_H
#define HYPERBLADE_PARTICLESYSTEM_H

#include "engine/coachobject.h"
#include "engine/gameengine.h"

#include "common/compat.h"
#include "common/staticassert.h"
#include "engine/engineobject.h"
#include "engine/particle.h"
#include "math/vector.h"

// The three corners of one shard's face, the offsets its particle is drawn at.
struct ParticleRow
{
    Vec3 aCorners[3];

    ParticleRow()
    {
        int nCorner;

        for (nCorner = 0; nCorner < 3; nCorner++)
        {
            aCorners[nCorner].z = 0.0f;
            aCorners[nCorner].y = 0.0f;
            aCorners[nCorner].x = 0.0f;
        }
    }
};

// The explosion entity: a hundred shard particles posed by rows copied off the model's faces.
// Slot three returns the transform and slot twenty five applies the render-slot pose.
// VTABLE: HYPERX 0x004b3c88
class ParticleSystem : public EngineObject
{
public:
    ParticleSystem(int nSlot, class ObjectDef* pDef);

    void Deactivate();

    void SetOrientation(const Vec3* pPosition, const Vec3* pDirection);
    void SpawnParticles(float flSpread, float flRise);
    void SpawnParticlesUniform(float flSpread, float flRise);
    void Update(float flDt);
    // Update is its only caller.
    void ApplyToMesh();

    ParticleRow m_aRows[PARTICLE_ROW_SETS * PARTICLE_ROWS_PER_SET]; // 0x00d14

    Particle m_aParticles[100]; // 0x01b24
    int      m_nParticleCount;  // 0x02c54
    int      m_bAnyAlive;       // 0x02c58
};

// SYNTHETIC: HYPERX 0x00474e70
// ParticleSystem::`scalar deleting destructor'

// SYNTHETIC: HYPERX 0x00474e90
// ParticleSystem::~ParticleSystem

STATIC_ASSERT(sizeof(ParticleSystem) == 0x2c5c);

inline ParticleSystem* GameEngine::GetParticleSystem(int nSlot) const
{
    return (ParticleSystem*)m_ppSlots[nSlot];
}

#endif
