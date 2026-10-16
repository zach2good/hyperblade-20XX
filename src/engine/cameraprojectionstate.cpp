#include "common/compat.h"
#include "cameraprojectionstate.h"
#include "math/transformobject.h"
#include "globals.h"
#include "engineglobals.h"
#include "common/staticinit.h"
#include "clipsphere.h"
#include "gameengine.h"
#include "math/vector.h"
#include "math/mathglobals.h"
#include <math.h>
#include <algorithm>

// FUNCTION: HYPERX 0x00422d60
void CameraProjectionState::ResetProjection(LayoutState* pViewport, float flFov)
{
    InitializeProjection(m_bHasRow3, pViewport, flFov);
}

// FUNCTION: HYPERX 0x00422d80
void CameraProjectionState::InitializeProjection(char cFlag, LayoutState* pViewport, float flFov)
{
    // Qualified, because the original calls the base's body directly rather than through slot two.
    TransformObject::Reset(cFlag);

    CreateProjectionState(cFlag, pViewport, flFov);
}

// FUNCTION: HYPERX 0x00422db0
float CameraProjectionState::GetHalfFov()
{
    return m_flHalfFov;
}

// FUNCTION: HYPERX 0x004303b0
void CameraProjectionState::CreateProjectionState(char cFlag, LayoutState* pViewport, float flFov)
{
    m_flCmPerPixel    = g_flDotPitchCmProjection;
    m_flPixelsPerCm   = g_flScreenDotsPerCmProjection;
    m_flHalfWidthCm   = g_flZeroProjection;
    m_flHalfHeightCm  = g_flZeroProjection;
    m_flHalfHeightPxB = g_flZeroProjection;
    m_flHalfWidthPxB  = g_flZeroProjection;
    m_flHalfHeightPxA = g_flZeroProjection;
    m_flHalfWidthPxA  = g_flZeroProjection;
    m_flTanHalfFov    = g_flZeroProjection;
    m_flHalfFov       = g_flZeroProjection;
    m_flOffsetYPx     = g_flZeroProjection;
    m_flOffsetXPx     = g_flZeroProjection;
    m_flNearClip      = g_flZeroProjection;
    m_flFarClip       = g_flZeroProjection;
    m_vEye.x = m_vEye.y = m_vEye.z = g_flZeroProjection;
    m_bHasRow3                     = cFlag;
    m_cUnusedB                     = 0;

    Configure(pViewport, flFov, 1000.0f);
}

// Assumes a 72-dot-per-inch screen for the centimetre conversion.

// FUNCTION: HYPERX 0x004304a0
void CameraProjectionState::Configure(LayoutState* pView, float flHalfFov, float flFarClip)
{
    m_flHalfWidthPx = m_flHalfWidthPxB = m_flHalfWidthPxA = pView->nPrevWidth * 0.5f;
    m_flHalfHeightPx = m_flHalfHeightPxB = m_flHalfHeightPxA = pView->nPrevHeight * 0.5f;

    m_flCmPerPixel = 1.0f / (float)(72.0 / g_flCmPerInchProjection);

    m_flHalfHeightCm = m_flCmPerPixel * m_flHalfHeightPxA;
    m_flHalfWidthCm  = m_flCmPerPixel * m_flHalfWidthPxA;

    m_flOffsetXPx = pView->nPrevWidth * 0.5f;
    m_flOffsetYPx = pView->nPrevHeight * 0.5f;

    m_flHalfFov = flHalfFov;
    m_flFarClip = flFarClip;

    const double dTan = tan(flHalfFov);

    m_dTanHalfFov = dTan;

    const double dSin = sin(flHalfFov);

    m_dSinHalfFov = dSin;

    const double dCos = cos(flHalfFov);

    m_dCosHalfFov = dCos;

    if (m_bHasRow3 != 0)
    {
        m_flTanHalfFov = (float)dTan;
        m_flNearClip   = 1.0f;
        m_clip.flMinX  = 0.0f;
        m_clip.flMaxX  = pView->flPrevRight;
        m_clip.flMinY  = 0.0f;
        m_clip.flMaxY  = pView->flPrevBottom;
        m_clip.flMinZ  = 0.0f;
        m_clip.flMaxZ  = flFarClip;
    }
    else
    {
        m_flNearClip = (float)(m_flHalfWidthCm * dCos / dSin);
    }

    BuildMatrix();
}

inline void CameraProjectionState::StoreTranslate(CameraProjectionState* pState, float flX, float flY, float flZ)
{
    pState->m_vTranslate.x = flX;
    pState->m_vTranslate.y = flY;
    pState->m_vTranslate.z = flZ;
}

// FUNCTION: HYPERX 0x00430620
void CameraProjectionState::SetEyePoint(const Vec3* pEye)
{
    m_vEye = *pEye;

    StoreTranslate(this, -pEye->x, -pEye->y, -pEye->z);

    ApplyTranslation();
}

// FUNCTION: HYPERX 0x00430660
void CameraProjectionState::GetEyePoint(Vec3* pOut)
{
    pOut->x = m_vEye.x;
    pOut->y = m_vEye.y;
    pOut->z = m_vEye.z;
}

// Maps camera space straight to pixels; the aspect fix is on the perspective path only.

// FUNCTION: HYPERX 0x00430690
void CameraProjectionState::BuildMatrix()
{
    if (m_bHasRow3 == 0)
    {
        const float flScaleX = m_flNearClip / m_flHalfWidthCm;

        m_flCotX = flScaleX;

        const float flScaleY = m_flNearClip / m_flHalfHeightCm;

        m_flCotY = flScaleY;

        m_flTanX = 1.0f / flScaleX;
        m_flTanY = 1.0f / flScaleY;

        const float* pTanX = &m_flTanX;
        const float* pTanY = &m_flTanY;

        m_flTanXSquared = *pTanX * *pTanX;
        m_flTanYSquared = *pTanY * *pTanY;

        if (m_bIdentity != 0)
        {
            m_a[0][0] = flScaleX;
            m_a[1][1] = flScaleY;
        }
        else
        {
            m_a[0][0] = flScaleX * m_a[0][0];
            m_a[0][1] = flScaleX * m_a[0][1];
            m_a[0][2] = flScaleX * m_a[0][2];
            m_a[0][3] = flScaleX * m_a[0][3];
            m_a[1][0] = flScaleY * m_a[1][0];
            m_a[1][1] = flScaleY * m_a[1][1];
            m_a[1][2] = flScaleY * m_a[1][2];
            m_a[1][3] = flScaleY * m_a[1][3];
        }
    }
    else
    {
        const float flDepth = m_flFarClip - m_flNearClip;
        const float flReach = m_flFarClip * m_flTanHalfFov;

        const float flDepthScale = flReach / (flDepth * m_flNearClip);
        const float flDepthBias  = -(flReach / flDepth);
        const float flWPerZ      = m_flTanHalfFov / m_flNearClip;

        // The field of view was set for 4:3, so a wider view keeps its height and sees more at the sides.
        const float flFocalPx = (std::min)(m_flHalfWidthPx, m_flHalfHeightPx * 4.0f / 3.0f);

        if (m_bIdentity != 0)
        {
            m_a[0][0] = flFocalPx;
            m_a[1][1] = m_flHalfHeightPx;
            m_a[0][2] = flWPerZ * m_flOffsetXPx;
            m_a[2][2] = flDepthScale;
            m_a[2][3] = flDepthBias;
            m_a[3][2] = flWPerZ;
            m_a[1][2] = flWPerZ * m_flOffsetYPx;

            m_a[3][3] = g_flZeroProjection;
        }
        else
        {
            TransformObject objProjection(1);

            objProjection.m_a[0][0] = flFocalPx;
            objProjection.m_a[1][1] = m_flHalfHeightPx;
            objProjection.m_a[0][2] = flWPerZ * m_flOffsetXPx;
            objProjection.m_a[2][2] = flDepthScale;
            objProjection.m_a[2][3] = flDepthBias;
            objProjection.m_a[3][2] = flWPerZ;
            objProjection.m_a[1][2] = flWPerZ * m_flOffsetYPx;

            objProjection.m_a[3][3] = g_flZeroProjection;

            Compose(&objProjection);
        }

        TransformObject objAspect(0);

        objAspect.m_vScale.y = flFocalPx / m_flHalfHeightPx;
        objAspect.m_vScale.x = 1.0f;
        objAspect.m_vScale.z = 1.0f;

        objAspect.ApplyScale();

        Compose(&objAspect);
    }

    MarkChanged();
}

// An identity matrix is written as the rotation rather than multiplied into.

// FUNCTION: HYPERX 0x00430960
void CameraProjectionState::RotateY(float flAngle)
{
    const float flCos = (float)cos(flAngle);
    m_rotY.flCos      = flCos;

    flAngle      = (float)sin(flAngle);
    m_rotY.flSin = flAngle;

    if (m_bIdentity != 0)
    {
        m_a[2][2] = flCos;
        m_a[0][0] = flCos;
        m_a[2][0] = -flAngle;
        m_a[0][2] = flAngle;
    }
    else
    {
        double dOld = m_a[0][0];
        m_a[0][0]   = flCos * dOld - flAngle * m_a[0][2];
        m_a[0][2]   = (flCos * m_a[0][2]) + flAngle * dOld;

        dOld      = m_a[1][0];
        m_a[1][0] = flCos * dOld - flAngle * m_a[1][2];
        m_a[1][2] = flCos * m_a[1][2] + (flAngle * dOld);

        dOld      = m_a[2][0];
        m_a[2][0] = flCos * dOld - flAngle * m_a[2][2];
        m_a[2][2] = flCos * m_a[2][2] + flAngle * dOld;

        m_a[0][3] = m_a[0][0] * GetTranslateX() + (m_a[0][1] * m_vTranslate.y) + GetElement(0, 2) * m_vTranslate.z;
        m_a[1][3] = (GetElement(1, 0) * m_vTranslate.x + m_a[1][1] * m_vTranslate.y) + m_a[1][2] * m_vTranslate.z;
        m_a[2][3] = GetElement(2, 0) * m_vTranslate.x + m_a[2][1] * m_vTranslate.y + m_a[2][2] * m_vTranslate.z;
    }

    MarkChanged();
}

// FUNCTION: HYPERX 0x00430a80
void CameraProjectionState::RotateX(float flAngle)
{
    const float flCos = (float)cos(flAngle);
    m_rotX.flCos      = flCos;

    flAngle      = (float)sin(flAngle);
    m_rotX.flSin = flAngle;

    if (m_bIdentity != 0)
    {
        m_a[2][2] = flCos;
        m_a[1][1] = flCos;
        m_a[1][2] = -flAngle;
        m_a[2][1] = flAngle;
    }
    else
    {
        double dOld = m_a[0][1];
        m_a[0][1]   = flCos * dOld + flAngle * m_a[0][2];
        m_a[0][2]   = flCos * m_a[0][2] - flAngle * dOld;

        dOld      = m_a[1][1];
        m_a[1][1] = flCos * dOld + flAngle * m_a[1][2];
        m_a[1][2] = flCos * m_a[1][2] - flAngle * dOld;

        dOld      = m_a[2][1];
        m_a[2][1] = flCos * dOld + flAngle * m_a[2][2];
        m_a[2][2] = flCos * m_a[2][2] - flAngle * dOld;

        m_a[0][3] = (m_a[0][0] * m_vTranslate.x + GetElement(0, 1) * m_vTranslate.y) + m_a[0][2] * m_vTranslate.z;
        m_a[1][3] = (m_a[1][0] * m_vTranslate.x + GetElement(1, 2) * m_vTranslate.z) + m_a[1][1] * m_vTranslate.GetY();
        m_a[2][3] = m_a[2][0] * m_vTranslate.x + GetElement(2, 1) * m_vTranslate.y + m_a[2][2] * m_vTranslate.z;
    }

    MarkChanged();
}

// FUNCTION: HYPERX 0x00430ba0
void CameraProjectionState::RotateZ(float flAngle)
{
    const float flCos = (float)cos(flAngle);
    m_rotZ.flCos      = flCos;

    flAngle      = (float)sin(flAngle);
    m_rotZ.flSin = flAngle;

    if (m_bIdentity != 0)
    {
        m_a[1][1] = flCos;
        m_a[0][0] = flCos;
        m_a[0][1] = -flAngle;
        m_a[1][0] = flAngle;
    }
    else
    {
        double dOld = m_a[0][0];
        m_a[0][0]   = flCos * dOld + flAngle * m_a[0][1];
        m_a[0][1]   = flCos * m_a[0][1] - flAngle * dOld;

        dOld      = m_a[1][0];
        m_a[1][0] = flCos * dOld + flAngle * m_a[1][1];
        m_a[1][1] = flCos * m_a[1][1] - flAngle * dOld;

        dOld      = m_a[2][0];
        m_a[2][0] = flCos * dOld + flAngle * m_a[2][1];
        m_a[2][1] = flCos * m_a[2][1] - flAngle * dOld;

        m_a[0][3] = m_a[0][0] * m_vTranslate.x + (m_a[0][1] * m_vTranslate.y) + m_a[0][2] * m_vTranslate.z;
        m_a[1][3] = GetElement(1, 0) * m_vTranslate.x + m_a[1][1] * m_vTranslate.y + m_a[1][2] * m_vTranslate.z;
        m_a[2][3] = GetElement(2, 0) * m_vTranslate.x + (m_a[2][1] * m_vTranslate.y + m_a[2][2] * m_vTranslate.z);
    }

    MarkChanged();
}

// The new translation also goes, negated, to m_vEye.

// FUNCTION: HYPERX 0x00430cc0
void CameraProjectionState::SubtractTranslation(float flX, float flY, float flZ)
{
    const float flNewX = m_vTranslate.x - flX;

    m_vTranslate.x = flNewX;
    m_a[0][3]      = flNewX;

    const float flNewY = m_vTranslate.y - flY;

    m_vTranslate.y = flNewY;
    m_a[1][3]      = flNewY;

    const float flNewZ = m_vTranslate.z - flZ;

    m_vTranslate.z = flNewZ;
    m_a[2][3]      = flNewZ;

    m_vEye.x = -flNewX;
    m_vEye.y = -flNewY;
    m_vEye.z = -flNewZ;

    MarkChanged();
}
