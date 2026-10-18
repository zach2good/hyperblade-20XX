#include "common/compat.h"
#include "math/mathops.h"
#include "quat.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "transformobject.h"
#include <cmath>
#include "app/gameapp.h"
#include "slerpseg.h"

extern const float g_flAnimBlendZero;

// Clears one of the animation binding's twenty 68-byte blend slots.
// FUNCTION: HYPERX 0x00425480
SlerpSeg::SlerpSeg()
{
    m_flTheta = g_flAnimBlendZero;
    m_q0.z    = g_flAnimBlendZero;
    m_q0.y    = g_flAnimBlendZero;
    m_q0.x    = g_flAnimBlendZero;
    m_q0.w    = g_flAnimBlendZero;
    m_q1.z    = g_flAnimBlendZero;
    m_q1.y    = g_flAnimBlendZero;
    m_q1.x    = g_flAnimBlendZero;
    m_q1.w    = g_flAnimBlendZero;
    m_q0Div.w = g_flAnimBlendZero;
    m_q0Div.x = g_flAnimBlendZero;
    m_q0Div.y = g_flAnimBlendZero;
    m_q0Div.z = g_flAnimBlendZero;
    m_q1Div.w = g_flAnimBlendZero;
    m_q1Div.x = g_flAnimBlendZero;
    m_q1Div.y = g_flAnimBlendZero;
    m_q1Div.z = g_flAnimBlendZero;
}

// A half angle under a thousandth of a radian stores zero, which tells Eval to do nothing.

// FUNCTION: HYPERX 0x0042e360
void SlerpSeg::Init(TransformObject* pStartMat, TransformObject* pEndMat)
{
    float flDot;

    m_q0.FromMatrix(pStartMat);
    m_q1.FromMatrix(pEndMat);

    flDot = m_q0.w * m_q1.w + m_q0.x * m_q1.x + m_q0.y * m_q1.y + m_q0.z * m_q1.z;

    if (flDot < 0.0f)
    {
        m_q1.w = -m_q1.w;
        m_q1.x = -m_q1.x;
        m_q1.y = -m_q1.y;
        m_q1.z = -m_q1.z;

        flDot = m_q0.w * m_q1.w + m_q0.x * m_q1.x + m_q0.y * m_q1.y + m_q0.z * m_q1.z;
    }

    const double dTheta = Math::Acos(flDot);
    m_flTheta           = (float)dTheta;

    if (dTheta < 0.001)
    {
        m_flTheta = 0.0f;
        return;
    }

    const float flSin = (float)sin(dTheta);

    if (flSin == 0.0f)
    {
        // STRING: HYPERX 0x004bcf10
        g_gameApp.Fatal("0 denom in InitSlerpSeg\n");
    }

    const float flInvSin = g_flQuatOne / flSin;

    m_q0Div.w = flInvSin * m_q0.w;
    m_q0Div.x = flInvSin * m_q0.x;
    m_q0Div.y = flInvSin * m_q0.y;
    m_q0Div.z = flInvSin * m_q0.z;
    m_q1Div.w = flInvSin * m_q1.w;
    m_q1Div.x = flInvSin * m_q1.x;
    m_q1Div.y = flInvSin * m_q1.y;
    m_q1Div.z = flInvSin * m_q1.z;
}

// Returns zero and writes nothing for a zero angle; Init already divided both by sin(theta).

// FUNCTION: HYPERX 0x0042e480
bool SlerpSeg::Eval(float flAt)
{
    float flWeightFrom;

    if (m_flTheta == 0.0f)
    {
        return false;
    }

    const float flWeightTo = (float)sin(flAt * m_flTheta);
    flWeightFrom           = (float)sin(m_flTheta - flAt * m_flTheta);

    m_q0.w = flWeightTo * m_q1Div.w + flWeightFrom * m_q0Div.w;
    m_q0.x = flWeightTo * m_q1Div.x + flWeightFrom * m_q0Div.x;
    m_q0.y = flWeightTo * m_q1Div.y + flWeightFrom * m_q0Div.y;
    m_q0.z = flWeightTo * m_q1Div.z + flWeightFrom * m_q0Div.z;

    return true;
}
