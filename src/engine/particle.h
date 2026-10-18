#ifndef HYPERBLADE_PARTICLE_H
#define HYPERBLADE_PARTICLE_H

struct Vec3;

#include "devicector.h"

// The shard matrices are five sets of twenty rows; a particle steps through one set.
constexpr auto kParticleRowSets    = 5;
constexpr auto kParticleRowsPerSet = 20;

// One 44-byte particle record, initialised at random by one of two spreads.
class Particle
{
public:
    Particle()
    {
        m_vPosition.z = 0.0f;
        m_vPosition.y = 0.0f;
        m_vPosition.x = 0.0f;
        m_vVelocity.z = 0.0f;
        m_vVelocity.y = 0.0f;
        m_vVelocity.x = 0.0f;
        m_flLife      = 0.0f;
        m_flRowRate   = 0.0f;
    }

    void InitRandom(float flSpread, float flRise);
    void InitRandomUniform(float flSpread, float flRise);
    void AdvanceRow(float flDelta);
    void IntegrateStep(const Vec3* pAccel, float flDt);

    Vec3  m_vVelocity; // 0x000
    Vec3  m_vPosition; // 0x00c
    float m_flRowRate; // 0x018
    float m_flLife;    // 0x01c
    float m_flSpan;    // 0x020
    float m_flRow;     // 0x024
    int   m_nSet;      // 0x028
};

#endif
