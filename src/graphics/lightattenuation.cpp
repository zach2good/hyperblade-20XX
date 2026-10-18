// The light attenuation the debug HUD reports: an inverted quadratic in distance, clamped at zero.

#include "common/compat.h"
#include "globals.h"
#include "common/staticinit.h"

extern const float g_flDebugLightDistance;

// GLOBAL: HYPERX 0x004c67bc
extern const float g_flLightAttenConstant;
const float        g_flLightAttenConstant = 0.25f;

// GLOBAL: HYPERX 0x004c67c0
extern const float g_flLightAttenLinear;
const float        g_flLightAttenLinear = 0.25f;

// GLOBAL: HYPERX 0x004c67c4
extern const float g_flLightAttenQuadratic;
const float        g_flLightAttenQuadratic = 0.5f;

// GLOBAL: HYPERX 0x004b3e10
extern const float g_flAttenFloor;
const float        g_flAttenFloor = 0.0f;

namespace LightAttenuation
{

inline float Compute()
{
    const auto flAtten = g_flOne / ((g_flDebugLightDistance * g_flLightAttenQuadratic + g_flLightAttenLinear) * g_flDebugLightDistance + g_flLightAttenConstant);

    if (flAtten > g_flAttenFloor)
    {
        return flAtten;
    }

    return g_flAttenFloor;
}

} // namespace LightAttenuation

float g_flLightAttenuation     = LightAttenuation::Compute();
float g_flScaledLightIntensity = g_flLightIntensityScale * g_flLightAttenuation;
