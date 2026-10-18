#include "common/compat.h"
#include "particle.h"
#include "globals.h"
#include <cstdlib>
#include <cmath>
#include "math/mathops.h"
#include "math/randroll.h"

// The spin is rolled in whole degrees a second.
#define PARTICLE_ROW_RATE_MAX 100.0f

// A faster particle lives longer: the vertical roll sets both life and span.
// FUNCTION: HYPERX 0x004127f0
void Particle::InitRandom(float flSpread, float flRise)
{
    m_vPosition.x = m_vPosition.y = m_vPosition.z = 0.0f;

    m_vVelocity.x = Math::RandSigned() * flSpread;
    m_vVelocity.z = Math::RandSigned() * flSpread;

    const auto flUpRoll = Math::RandMagnitude();

    const auto flVertical = flUpRoll * flRise;
    m_vVelocity.y         = flVertical;
    const auto flSpan     = flVertical * 0.0625f;
    m_flSpan              = flSpan;
    m_flLife              = flSpan;

    const auto flRateRoll = Math::RandMagnitude();

    const auto nRowRate = static_cast<int>(flRateRoll * PARTICLE_ROW_RATE_MAX + 0.5);
    m_flRowRate         = static_cast<float>(nRowRate);

    m_nSet = Math::RandInt(0, kParticleRowSets - 1);

    const auto nRow = Math::RandInt(0, kParticleRowsPerSet - 1);
    m_flRow         = static_cast<float>(nRow);
}

// Unlike InitRandom, half the particles leave downwards and they live half as long.
// FUNCTION: HYPERX 0x00412900
void Particle::InitRandomUniform(float flSpread, float flRise)
{
    m_vPosition.x = m_vPosition.y = m_vPosition.z = 0.0f;

    m_vVelocity.x = Math::RandSigned() * flSpread;
    m_vVelocity.z = Math::RandSigned() * flSpread;

    auto flVertical = Math::RandSigned() * flRise;
    if (flVertical <= 0.0f)
    {
        m_vVelocity.y = flVertical;
        flVertical    = -flVertical;
    }
    else
    {
        m_vVelocity.y = flVertical;
    }

    const auto flSpan = flVertical * 0.03125f;
    m_flSpan          = flSpan;
    m_flLife          = flSpan;

    const auto flRateRoll = Math::RandMagnitude();

    const auto nRowRate = static_cast<int>(flRateRoll * PARTICLE_ROW_RATE_MAX + 0.5);
    m_flRowRate         = static_cast<float>(nRowRate);

    m_nSet = Math::RandInt(0, kParticleRowSets - 1);

    const auto nRow = Math::RandInt(0, kParticleRowsPerSet - 1);
    m_flRow         = static_cast<float>(nRow);
}

// FUNCTION: HYPERX 0x00412a10
void Particle::IntegrateStep(const Vec3* pAccel, float flDt)
{
    Vec3 vStep;

    vStep.x = flDt * pAccel->x;
    vStep.y = flDt * pAccel->y;
    vStep.z = flDt * pAccel->z;

    m_vVelocity.x = vStep.x + m_vVelocity.x;
    m_vVelocity.y = vStep.y + m_vVelocity.y;
    m_vVelocity.z = vStep.z + m_vVelocity.z;

    vStep.x = m_vVelocity.x * flDt;
    vStep.y = m_vVelocity.y * flDt;
    vStep.z = m_vVelocity.z * flDt;

    m_vPosition.x = vStep.x + m_vPosition.x;
    m_vPosition.y = vStep.y + m_vPosition.y;
    m_vPosition.z = vStep.z + m_vPosition.z;

    AdvanceRow(flDt * m_flRowRate);

    m_flLife -= flDt;

    if (m_flLife < 0.0f)
    {
        m_flLife = 0.0f;
    }
}

// FUNCTION: HYPERX 0x00412ad0
void Particle::AdvanceRow(float flDelta)
{
    m_flRow += flDelta;

    while (m_flRow >= kParticleRowsPerSet)
    {
        m_flRow -= kParticleRowsPerSet;
    }
}
