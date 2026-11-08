#include "common/compat.h"
#include "quat.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "transformobject.h"
#include <math.h>
#include "app/gameapp.h"
#include "slerpseg.h"

extern const float g_flAnimBlendZero;

// Clears one of the animation binding's twenty 68-byte blend slots.
// FUNCTION: HYPERX100 0x00439520
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

// FUNCTION: HYPERX100 0x004a2fa0
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

    const double dTheta = acos(flDot);
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

// FUNCTION: HYPERX100 0x004a3170
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

#ifdef HYPERX_V100
// FUNCTION: HYPERX100 0x004a2cb0
void SlerpSeg::InitFromQuats(const Quat* pQ0, Quat* pQ1)
{
    float flDot = pQ1->y * pQ0->y + pQ0->x * pQ1->x + pQ1->z * pQ0->z + pQ1->w * pQ0->w;

    if (flDot < 0.0f)
    {
        pQ1->w = -pQ1->w;
        pQ1->x = -pQ1->x;
        pQ1->y = -pQ1->y;
        pQ1->z = -pQ1->z;

        flDot = pQ1->y * pQ0->y + pQ1->z * pQ0->z + pQ1->x * pQ0->x + pQ1->w * pQ0->w;
    }

    m_flTheta = (float)acos(flDot);

    const double dSin = sin(m_flTheta);

    if (dSin == 0.0f)
    {
        // STRING: HYPERX100 0x0051bd98
        g_gameApp.Fatal("0 denom in InitSlerpSeg\n");
    }

    const float flInvSin = g_flQuatOne / (float)dSin;

    m_q0Div.w = pQ0->w * flInvSin;
    m_q0Div.x = pQ0->x * flInvSin;
    m_q0Div.y = pQ0->y * flInvSin;
    m_q0Div.z = pQ0->z * flInvSin;
    m_q1Div.w = pQ1->w * flInvSin;
    m_q1Div.x = pQ1->x * flInvSin;
    m_q1Div.y = pQ1->y * flInvSin;
    m_q1Div.z = pQ1->z * flInvSin;
}

// FUNCTION: HYPERX100 0x004a2e30
void SlerpSeg::InitInPlace()
{
    float flDot = m_q1Div.z * m_q0Div.z + m_q0Div.x * m_q1Div.x + m_q1Div.y * m_q0Div.y + m_q0Div.w * m_q1Div.w;

    if (flDot < 0.0f)
    {
        m_q1Div.w = -m_q1Div.w;
        m_q1Div.x = -m_q1Div.x;
        m_q1Div.y = -m_q1Div.y;
        m_q1Div.z = -m_q1Div.z;

        flDot = m_q1Div.y * m_q0Div.y + m_q1Div.z * m_q0Div.z + m_q1Div.w * m_q0Div.w + m_q1Div.x * m_q0Div.x;
    }

    m_flTheta = (float)acos(flDot);

    const double dSin = sin(m_flTheta);

    if (dSin == 0.0f)
    {
        g_gameApp.Fatal("0 denom in InitSlerpSeg\n");
    }

    const float flInvSin = g_flQuatOne / (float)dSin;

    m_q0Div.w = m_q0Div.w * flInvSin;
    m_q0Div.x = m_q0Div.x * flInvSin;
    m_q0Div.y = m_q0Div.y * flInvSin;
    m_q0Div.z = m_q0Div.z * flInvSin;
    m_q1Div.w = m_q1Div.w * flInvSin;
    m_q1Div.x = m_q1Div.x * flInvSin;
    m_q1Div.y = m_q1Div.y * flInvSin;
    m_q1Div.z = m_q1Div.z * flInvSin;
}
#endif
