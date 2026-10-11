#include "common/compat.h"
#include "transformobject.h"
#include "vector.h"
#include "mathglobals.h"
#include "engine/cameraprojectionstate.h"
#include <string.h>
#include "math/mathops.h"

// FUNCTION: HYPERX 0x00403110
void TransformObject::TransformPoint(Vec3 vPoint, Vec3* pOut)
{
    pOut->x = m_a[0][0] * vPoint.x + m_a[0][1] * vPoint.y + m_a[0][2] * vPoint.z + m_a[0][3];
    pOut->y = m_a[1][0] * vPoint.x + m_a[1][1] * vPoint.y + m_a[1][2] * vPoint.z + m_a[1][3];
    pOut->z = m_a[2][0] * vPoint.x + m_a[2][1] * vPoint.y + m_a[2][2] * vPoint.z + m_a[2][3];
}

// FUNCTION: HYPERX 0x00406de0
void TransformObject::TransformDirection(Vec3 vec, Vec3* pOut)
{
    float flRow = m_a[0][2] * vec.z;
    flRow += m_a[0][1] * vec.y;
    pOut->x = flRow + m_a[0][0] * vec.x;

    flRow = m_a[1][2] * vec.z;
    flRow += m_a[1][1] * vec.y;
    pOut->y = flRow + m_a[1][0] * vec.x;

    flRow = m_a[2][2] * vec.z;
    flRow += m_a[2][1] * vec.y;
    pOut->z = flRow + m_a[2][0] * vec.x;
}

// The direction applies the transpose, which for a rotation is the inverse.
// FUNCTION: HYPERX 0x00406e40
void TransformObject::TransformDirectionByRows(Vec3 vDir, Vec3* pOut)
{
    float flRow = m_a[2][0] * vDir.z;
    flRow += m_a[1][0] * vDir.y;
    pOut->x = flRow + m_a[0][0] * vDir.x;

    flRow = m_a[2][1] * vDir.z;
    flRow += m_a[1][1] * vDir.y;
    pOut->y = flRow + m_a[0][1] * vDir.x;

    flRow = m_a[2][2] * vDir.z;
    flRow += m_a[1][2] * vDir.y;
    pOut->z = flRow + m_a[0][2] * vDir.x;
}

// Resets the transform through its own third slot, handing it the byte it already holds.

// FUNCTION: HYPERX 0x00416e40
void TransformObject::ResetToIdentity()
{
    Reset(m_bHasRow3);
}

// FUNCTION: HYPERX 0x00416e50
Vec3* TransformObject::GetTranslationPtr()
{
    return &m_vTranslate;
}

// Neither updates m_vTranslate, so the two copies drift apart.
// FUNCTION: HYPERX 0x00416e60
void TransformObject::AddTranslation(float flX, float flY, float flZ)
{
    m_a[0][3] += flX;
    m_a[1][3] += flY;
    m_a[2][3] += flZ;

    MarkChanged();
}

// FUNCTION: HYPERX 0x00416ea0
void TransformObject::AddTranslationVec(const Vec3* pDelta)
{
    m_a[0][3] += pDelta->x;
    m_a[1][3] += pDelta->y;
    m_a[2][3] += pDelta->z;

    MarkChanged();
}

// FUNCTION: HYPERX 0x00416ee0
Vec3* TransformObject::GetScaleVec(Vec3* pOut)
{
    *pOut = m_vScale;

    return pOut;
}

// FUNCTION: HYPERX 0x00416f00
MatrixRow* TransformObject::GetMatrixPtr()
{
    return m_aMatrixRows;
}

// FUNCTION: HYPERX 0x00426a60
TransformObject::~TransformObject()
{
}

// FUNCTION: HYPERX 0x00426a70
void TransformObject::Reset(char bHasRow3)
{
    memset(m_a, 0, sizeof(m_a));

    m_a[3][3] = g_flTransformOne;
    m_a[2][2] = g_flTransformOne;
    m_a[1][1] = g_flTransformOne;
    m_a[0][0] = g_flTransformOne;

    m_bIdentity = 1;

    m_vScale.x = m_vScale.y = m_vScale.z = g_flTransformOne;

    m_vTranslate.x = m_vTranslate.y = m_vTranslate.z = g_flTransformZero;
    m_rotZ.flCos                                     = g_flTransformZero;
    m_rotZ.flSin                                     = g_flTransformZero;
    m_rotX.flCos                                     = g_flTransformZero;
    m_rotX.flSin                                     = g_flTransformZero;
    m_rotY.flCos                                     = g_flTransformZero;
    m_rotY.flSin                                     = g_flTransformZero;

    m_bHasRow3 = bHasRow3;
    m_nSerial  = 0;
}

// Scales per column where 0x00426c20 scales per row; only their identity arms agree.

// FUNCTION: HYPERX 0x00426b20
void TransformObject::ApplyScale()
{
    if (m_bIdentity != 0)
    {
        m_a[0][0] = m_vScale.x;
        m_a[1][1] = m_vScale.y;
        m_a[2][2] = m_vScale.z;

        if (m_vScale.x != 1.0 || m_vScale.y != 1.0 || m_vScale.z != 1.0)
        {
            MarkChanged();
        }

        return;
    }

    m_a[0][0] *= m_vScale.x;
    m_a[0][1] = m_vScale.y * m_a[0][1];
    m_a[0][2] = m_vScale.z * m_a[0][2];
    m_a[1][0] = m_vScale.x * m_a[1][0];
    m_a[1][1] *= m_vScale.y;
    m_a[1][2] = m_vScale.z * m_a[1][2];
    m_a[2][0] *= m_vScale.x;
    m_a[2][1] *= m_vScale.y;
    m_a[2][2] *= m_vScale.z;

    if (m_bHasRow3 != 0)
    {
        m_a[3][0] = m_vScale.x * m_a[3][0];
        m_a[3][1] = m_vScale.y * m_a[3][1];
        m_a[3][2] = m_vScale.z * m_a[3][2];
    }

    MarkChanged();
}

// Scales per row where 0x00426b20 scales per column; only their identity arms agree.

// FUNCTION: HYPERX 0x00426c20
void TransformObject::ApplyScaleToTarget(float flScaleX, float flScaleY, float flScaleZ, TransformObject* pDest)
{
    pDest->m_vScale.x = flScaleX;
    pDest->m_vScale.y = flScaleY;
    pDest->m_vScale.z = flScaleZ;

    if (pDest->m_bIdentity != 0)
    {
        pDest->m_a[0][0] = flScaleX;
        pDest->m_a[1][1] = flScaleY;
        pDest->m_a[2][2] = flScaleZ;

        if (flScaleX != 1.0 || flScaleY != 1.0 || flScaleZ != 1.0)
        {
            pDest->MarkChanged();
        }

        return;
    }

    pDest->m_a[0][0] = flScaleX * m_a[0][0];
    pDest->m_a[0][1] = flScaleX * m_a[0][1];
    pDest->m_a[0][2] = flScaleX * m_a[0][2];
    pDest->m_a[1][0] = flScaleY * m_a[1][0];
    pDest->m_a[1][1] = flScaleY * m_a[1][1];
    pDest->m_a[1][2] = flScaleY * m_a[1][2];
    pDest->m_a[2][0] = flScaleZ * m_a[2][0];
    pDest->m_a[2][1] = flScaleZ * m_a[2][1];
    pDest->m_a[2][2] = flScaleZ * m_a[2][2];

    if (m_bHasRow3 != 0)
    {
        pDest->m_a[3][0] = flScaleX * m_a[3][0];
        pDest->m_a[3][1] = flScaleY * m_a[3][1];
        pDest->m_a[3][2] = flScaleZ * m_a[3][2];
    }

    pDest->MarkChanged();
}

// A fresh matrix takes the vector as its translation; otherwise it folds into each row.

// FUNCTION: HYPERX 0x00426d40
void TransformObject::ApplyTranslation()
{
    if (m_bIdentity != 0)
    {
        m_a[0][3] = m_vTranslate.x;
        m_a[1][3] = m_vTranslate.y;
        m_a[2][3] = m_vTranslate.z;
    }
    else
    {
        m_a[0][3] = m_a[0][0] * m_vTranslate.x + m_a[0][1] * m_vTranslate.y + m_a[0][2] * m_vTranslate.z + m_a[0][3];
        m_a[1][3] = m_a[1][0] * m_vTranslate.x + m_a[1][1] * m_vTranslate.y + m_a[1][2] * m_vTranslate.z + m_a[1][3];
        m_a[2][3] = m_a[2][0] * m_vTranslate.x + m_a[2][1] * m_vTranslate.y + m_a[2][2] * m_vTranslate.z + m_a[2][3];

        if (m_bHasRow3 != 0)
        {
            m_a[3][3] = m_a[3][0] * m_vTranslate.x + m_a[3][1] * m_vTranslate.y + m_a[3][2] * m_vTranslate.z + m_a[3][3];
        }
    }

    MarkChanged();
}

// The translation lives twice, as column 3 and as m_vTranslate; both are written.

// FUNCTION: HYPERX 0x00426df0
void TransformObject::SetTranslation(float flX, float flY, float flZ)
{
    m_vTranslate.x = flX;
    m_vTranslate.y = flY;
    m_vTranslate.z = flZ;

    m_a[0][3] = flX;
    m_a[1][3] = flY;
    m_a[2][3] = flZ;

    MarkChanged();
}

// Also turns the fourth row, which the angle-taking version does not, and keeps the translation.

// FUNCTION: HYPERX 0x00426e40
void TransformObject::RotateAroundY()
{
    if (m_bIdentity != 0)
    {
        const float flFresh = m_rotY.flCos;
        m_a[2][2]           = flFresh;
        m_a[0][0]           = flFresh;
        m_a[2][0]           = -m_rotY.flSin;
        m_a[0][2]           = m_rotY.flSin;
    }
    else
    {
        const SinCos rot = m_rotY;

        double dOld = m_a[0][0];
        m_a[0][0]   = (float)(rot.flCos * dOld - rot.flSin * m_a[0][2]);
        m_a[0][2]   = (float)((rot.flCos * m_a[0][2]) + rot.flSin * dOld);

        dOld      = m_a[1][0];
        m_a[1][0] = (float)(rot.flCos * dOld - rot.flSin * m_a[1][2]);
        m_a[1][2] = (float)((rot.flCos * m_a[1][2]) + rot.flSin * dOld);

        dOld      = m_a[2][0];
        m_a[2][0] = (float)(rot.flCos * dOld - rot.flSin * m_a[2][2]);
        m_a[2][2] = (float)((rot.flCos * m_a[2][2]) + rot.flSin * dOld);

        if (m_bHasRow3 != 0)
        {
            dOld      = m_a[3][0];
            m_a[3][0] = (float)(rot.flCos * dOld - rot.flSin * m_a[3][2]);
            m_a[3][2] = (float)((rot.flCos * m_a[3][2]) + rot.flSin * dOld);
        }
    }

    MarkChanged();
}

// The two fourth-row flags pick one of four multiplies; only four by four calls Math::MatrixMultiplyInPlace.

// FUNCTION: HYPERX 0x00426f40
void TransformObject::Compose(const TransformObject* pOther)
{
    unsigned char cMode = 0;

    if (m_bHasRow3 != 0)
    {
        cMode = 2;
    }

    cMode |= pOther->m_bHasRow3;

    m_bHasRow3 = cMode > 0;

    switch (cMode)
    {
        case 0:
        {
            const float flRow0X = m_a[0][0];
            const float flRow0Y = m_a[0][1];
            const float flRow0Z = m_a[0][2];

            m_a[0][0] = flRow0X * pOther->m_a[0][0] + flRow0Z * pOther->m_a[2][0] + flRow0Y * pOther->m_a[1][0];
            m_a[0][1] = flRow0X * pOther->m_a[0][1] + (flRow0Z * pOther->m_a[2][1] + flRow0Y * pOther->m_a[1][1]);
            m_a[0][2] = flRow0X * pOther->m_a[0][2] + flRow0Z * pOther->m_a[2][2] + flRow0Y * pOther->m_a[1][2];
            m_a[0][3] = ((flRow0Z * pOther->m_a[2][3] + flRow0Y * pOther->m_a[1][3]) + flRow0X * pOther->m_a[0][3]) + m_a[0][3];

            const float flRow1X = m_a[1][0];
            const float flRow1Y = m_a[1][1];
            const float flRow1Z = m_a[1][2];

            m_a[1][0] = flRow1X * pOther->m_a[0][0] + flRow1Z * pOther->m_a[2][0] + flRow1Y * pOther->m_a[1][0];
            m_a[1][1] = flRow1X * pOther->m_a[0][1] + (flRow1Z * pOther->m_a[2][1] + flRow1Y * pOther->m_a[1][1]);
            m_a[1][2] = flRow1X * pOther->m_a[0][2] + flRow1Z * pOther->m_a[2][2] + flRow1Y * pOther->m_a[1][2];
            m_a[1][3] = ((flRow1Z * pOther->m_a[2][3] + flRow1Y * pOther->m_a[1][3]) + flRow1X * pOther->m_a[0][3]) + m_a[1][3];

            const float flRow2X = m_a[2][0];
            const float flRow2Y = m_a[2][1];
            const float flRow2Z = m_a[2][2];

            m_a[2][0] = flRow2X * pOther->m_a[0][0] + flRow2Z * pOther->m_a[2][0] + flRow2Y * pOther->m_a[1][0];
            m_a[2][1] = flRow2X * pOther->m_a[0][1] + (flRow2Z * pOther->m_a[2][1] + flRow2Y * pOther->m_a[1][1]);
            m_a[2][2] = flRow2X * pOther->m_a[0][2] + flRow2Z * pOther->m_a[2][2] + flRow2Y * pOther->m_a[1][2];
            m_a[2][3] = ((flRow2Z * pOther->m_a[2][3] + flRow2Y * pOther->m_a[1][3]) + flRow2X * pOther->m_a[0][3]) + m_a[2][3];

            break;
        }

        case 1:
        {
            const float flRow0X = m_a[0][0];
            const float flRow0Y = m_a[0][1];
            const float flRow0Z = m_a[0][2];
            const float flRow0W = m_a[0][3];

            m_a[0][0] = flRow0W * pOther->m_a[3][0] + flRow0X * pOther->m_a[0][0] + flRow0Z * pOther->m_a[2][0] + flRow0Y * pOther->m_a[1][0];
            m_a[0][1] = ((flRow0Z * pOther->m_a[2][1] + flRow0Y * pOther->m_a[1][1]) + flRow0W * pOther->m_a[3][1]) + flRow0X * pOther->m_a[0][1];
            m_a[0][2] = flRow0X * pOther->m_a[0][2] + flRow0W * pOther->m_a[3][2] + flRow0Z * pOther->m_a[2][2] + flRow0Y * pOther->m_a[1][2];
            m_a[0][3] = flRow0W * pOther->m_a[3][3] + (flRow0Z * pOther->m_a[2][3] + flRow0Y * pOther->m_a[1][3]) + flRow0X * pOther->m_a[0][3];

            const float flRow1X = m_a[1][0];
            const float flRow1Y = m_a[1][1];
            const float flRow1Z = m_a[1][2];
            const float flRow1W = m_a[1][3];

            m_a[1][0] = flRow1W * pOther->m_a[3][0] + flRow1X * pOther->m_a[0][0] + flRow1Z * pOther->m_a[2][0] + flRow1Y * pOther->m_a[1][0];
            m_a[1][1] = ((flRow1Z * pOther->m_a[2][1] + flRow1Y * pOther->m_a[1][1]) + flRow1W * pOther->m_a[3][1]) + flRow1X * pOther->m_a[0][1];
            m_a[1][2] = flRow1X * pOther->m_a[0][2] + flRow1W * pOther->m_a[3][2] + flRow1Z * pOther->m_a[2][2] + flRow1Y * pOther->m_a[1][2];
            m_a[1][3] = flRow1W * pOther->m_a[3][3] + (flRow1Z * pOther->m_a[2][3] + flRow1Y * pOther->m_a[1][3]) + flRow1X * pOther->m_a[0][3];

            const float flRow2X = m_a[2][0];
            const float flRow2Y = m_a[2][1];
            const float flRow2Z = m_a[2][2];
            const float flRow2W = m_a[2][3];

            m_a[2][0] = flRow2W * pOther->m_a[3][0] + flRow2X * pOther->m_a[0][0] + flRow2Z * pOther->m_a[2][0] + flRow2Y * pOther->m_a[1][0];
            m_a[2][1] = ((flRow2Z * pOther->m_a[2][1] + flRow2Y * pOther->m_a[1][1]) + flRow2W * pOther->m_a[3][1]) + flRow2X * pOther->m_a[0][1];
            m_a[2][2] = flRow2X * pOther->m_a[0][2] + flRow2W * pOther->m_a[3][2] + flRow2Z * pOther->m_a[2][2] + flRow2Y * pOther->m_a[1][2];
            m_a[2][3] = flRow2W * pOther->m_a[3][3] + (flRow2Z * pOther->m_a[2][3] + flRow2Y * pOther->m_a[1][3]) + flRow2X * pOther->m_a[0][3];

            m_a[3][0] = pOther->m_a[3][0];
            m_a[3][1] = pOther->m_a[3][1];
            m_a[3][2] = pOther->m_a[3][2];
            m_a[3][3] = pOther->m_a[3][3];
            break;
        }

        case 2:
        {
            const float flRow0X = m_a[0][0];
            const float flRow0Y = m_a[0][1];
            const float flRow0Z = m_a[0][2];

            m_a[0][0] = flRow0X * pOther->m_a[0][0] + flRow0Z * pOther->m_a[2][0] + flRow0Y * pOther->m_a[1][0];
            m_a[0][1] = flRow0X * pOther->m_a[0][1] + (flRow0Z * pOther->m_a[2][1] + flRow0Y * pOther->m_a[1][1]);
            m_a[0][2] = flRow0X * pOther->m_a[0][2] + flRow0Z * pOther->m_a[2][2] + flRow0Y * pOther->m_a[1][2];
            m_a[0][3] = ((flRow0Z * pOther->m_a[2][3] + flRow0Y * pOther->m_a[1][3]) + flRow0X * pOther->m_a[0][3]) + m_a[0][3];

            const float flRow1X = m_a[1][0];
            const float flRow1Y = m_a[1][1];
            const float flRow1Z = m_a[1][2];

            m_a[1][0] = flRow1X * pOther->m_a[0][0] + flRow1Z * pOther->m_a[2][0] + flRow1Y * pOther->m_a[1][0];
            m_a[1][1] = flRow1X * pOther->m_a[0][1] + (flRow1Z * pOther->m_a[2][1] + flRow1Y * pOther->m_a[1][1]);
            m_a[1][2] = flRow1X * pOther->m_a[0][2] + flRow1Z * pOther->m_a[2][2] + flRow1Y * pOther->m_a[1][2];
            m_a[1][3] = ((flRow1Z * pOther->m_a[2][3] + flRow1Y * pOther->m_a[1][3]) + flRow1X * pOther->m_a[0][3]) + m_a[1][3];

            // Rows two and three group their sums apart from rows zero and one.
            const float flRow2X = m_a[2][0];
            const float flRow2Y = m_a[2][1];
            const float flRow2Z = m_a[2][2];

            m_a[2][0] = (flRow2X * pOther->GetElement(0, 0) + flRow2Z * pOther->m_a[2][0]) + flRow2Y * pOther->m_a[1][0];
            m_a[2][1] = flRow2X * pOther->m_a[0][1] + (flRow2Z * pOther->m_a[2][1] + flRow2Y * pOther->m_a[1][1]);
            m_a[2][2] = (flRow2X * pOther->m_a[0][2] + flRow2Z * pOther->m_a[2][2]) + flRow2Y * pOther->m_a[1][2];
            m_a[2][3] = ((flRow2Z * pOther->m_a[2][3] + flRow2Y * pOther->m_a[1][3]) + flRow2X * pOther->m_a[0][3]) + m_a[2][3];

            const float flRow3X = m_a[3][0];
            const float flRow3Y = m_a[3][1];
            const float flRow3Z = m_a[3][2];

            m_a[3][0] = flRow3X * pOther->m_a[0][0] + flRow3Z * pOther->m_a[2][0] + flRow3Y * pOther->m_a[1][0];
            m_a[3][1] = flRow3X * pOther->m_a[0][1] + (flRow3Z * pOther->m_a[2][1] + flRow3Y * pOther->m_a[1][1]);
            m_a[3][2] = (flRow3X * pOther->m_a[0][2] + flRow3Z * pOther->m_a[2][2]) + flRow3Y * pOther->m_a[1][2];
            m_a[3][3] = ((flRow3Z * pOther->m_a[2][3] + flRow3Y * pOther->m_a[1][3]) + flRow3X * pOther->m_a[0][3]) + m_a[3][3];

            break;
        }

        case 3:
            Math::MatrixMultiplyInPlace(m_a, pOther->m_a);
            break;
    }

    MarkChanged();
}

// The same four shapes as the in-place compose, written into a third transform.

// FUNCTION: HYPERX 0x00427400
void TransformObject::Multiply(TransformObject* pResult, const TransformObject* pRhs) const
{
    float flAcc;
    float flTerm;

    unsigned char cMode = 0;

    if (m_bHasRow3 != 0)
    {
        cMode = 2;
    }

    cMode |= pRhs->m_bHasRow3;

    pResult->m_bHasRow3 = cMode > 0;

    switch (cMode)
    {
        case 0:
        {
            flTerm = pRhs->m_a[1][0];
            flTerm *= m_a[0][1];
            flAcc  = flTerm;
            flTerm = m_a[0][2];
            flTerm *= pRhs->m_a[2][0];
            flAcc += flTerm;
            flTerm = m_a[0][0];
            flTerm *= pRhs->m_a[0][0];
            pResult->m_a[0][0] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][1];
            flTerm *= m_a[0][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[1][1];
            flTerm *= m_a[0][1];
            flAcc += flTerm;
            flTerm = m_a[0][0];
            flTerm *= pRhs->m_a[0][1];
            pResult->m_a[0][1] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][2];
            flTerm *= m_a[0][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[1][2];
            flTerm *= m_a[0][1];
            flAcc += flTerm;
            flTerm = m_a[0][0];
            flTerm *= pRhs->m_a[0][2];
            pResult->m_a[0][2] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][3];
            flTerm *= m_a[0][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[1][3];
            flTerm *= m_a[0][1];
            flAcc += flTerm;
            flTerm = m_a[0][0];
            flTerm *= pRhs->m_a[0][3];
            pResult->m_a[0][3] = flAcc + flTerm;
            flTerm             = pRhs->m_a[0][0];
            flTerm *= m_a[1][0];
            flAcc  = flTerm;
            flTerm = m_a[1][2];
            flTerm *= pRhs->m_a[2][0];
            flAcc += flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][0];
            pResult->m_a[1][0] = flAcc + flTerm;
            flTerm             = m_a[1][2];
            flTerm *= pRhs->m_a[2][1];
            flAcc  = flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][1];
            flAcc += flTerm;
            flTerm = m_a[1][0];
            flTerm *= pRhs->m_a[0][1];
            pResult->m_a[1][1] = flAcc + flTerm;
            flTerm             = m_a[1][2];
            flTerm *= pRhs->m_a[2][2];
            flAcc  = flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][2];
            flAcc += flTerm;
            flTerm = m_a[1][0];
            flTerm *= pRhs->m_a[0][2];
            pResult->m_a[1][2] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][3];
            flTerm *= m_a[1][2];
            flAcc  = flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][3];
            flAcc += flTerm;
            flTerm = m_a[1][0];
            flTerm *= pRhs->m_a[0][3];
            pResult->m_a[1][3] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][0];
            flTerm *= m_a[2][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[1][0];
            flTerm *= m_a[2][1];
            flAcc += flTerm;
            flTerm = pRhs->m_a[0][0];
            flTerm *= m_a[2][0];
            pResult->m_a[2][0] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][1];
            flTerm *= m_a[2][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[1][1];
            flTerm *= m_a[2][1];
            flAcc += flTerm;
            flTerm = m_a[2][0];
            flTerm *= pRhs->m_a[0][1];
            pResult->m_a[2][1] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][2];
            flTerm *= m_a[2][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[1][2];
            flTerm *= m_a[2][1];
            flAcc += flTerm;
            flTerm = m_a[2][0];
            flTerm *= pRhs->m_a[0][2];
            pResult->m_a[2][2] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][3];
            flTerm *= m_a[2][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[1][3];
            flTerm *= m_a[2][1];
            flAcc += flTerm;
            flTerm = m_a[2][0];
            flTerm *= pRhs->m_a[0][3];
            pResult->m_a[2][3] = flAcc + flTerm;

            pResult->m_a[0][3] = m_a[0][3] + pResult->m_a[0][3];
            pResult->m_a[1][3] = m_a[1][3] + pResult->m_a[1][3];
            pResult->m_a[2][3] = m_a[2][3] + pResult->m_a[2][3];
            break;
        }

        case 1:
        {
            flTerm = m_a[0][2];
            flTerm *= pRhs->m_a[2][0];
            flAcc  = flTerm;
            flTerm = m_a[0][0];
            flTerm *= pRhs->m_a[0][0];
            flAcc += flTerm;
            flTerm = pRhs->m_a[1][0];
            flTerm *= m_a[0][1];
            pResult->m_a[0][0] = flAcc + flTerm;
            flTerm             = m_a[0][2];
            flTerm *= pRhs->m_a[2][1];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][1];
            flTerm *= m_a[0][0];
            flAcc += flTerm;
            flTerm = m_a[0][1];
            flTerm *= pRhs->m_a[1][1];
            pResult->m_a[0][1] = flAcc + flTerm;
            flTerm             = m_a[0][2];
            flTerm *= pRhs->m_a[2][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][2];
            flTerm *= m_a[0][0];
            flAcc += flTerm;
            flTerm = m_a[0][1];
            flTerm *= pRhs->m_a[1][2];
            pResult->m_a[0][2] = flAcc + flTerm;
            flTerm             = m_a[0][2];
            flTerm *= pRhs->m_a[2][3];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][3];
            flTerm *= m_a[0][0];
            flAcc += flTerm;
            flTerm = m_a[0][1];
            flTerm *= pRhs->m_a[1][3];
            pResult->m_a[0][3] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][0];
            flTerm *= m_a[1][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[1][0];
            flTerm *= m_a[1][1];
            flAcc += flTerm;
            flTerm = m_a[1][0];
            flTerm *= pRhs->m_a[0][0];
            pResult->m_a[1][0] = flAcc + flTerm;
            flTerm             = m_a[1][0];
            flTerm *= pRhs->m_a[0][1];
            flAcc  = flTerm;
            flTerm = m_a[1][2];
            flTerm *= pRhs->m_a[2][1];
            flAcc += flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][1];
            pResult->m_a[1][1] = flAcc + flTerm;
            flTerm             = m_a[1][0];
            flTerm *= pRhs->m_a[0][2];
            flAcc  = flTerm;
            flTerm = m_a[1][2];
            flTerm *= pRhs->m_a[2][2];
            flAcc += flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][2];
            pResult->m_a[1][2] = flAcc + flTerm;
            flTerm             = m_a[1][0];
            flTerm *= pRhs->m_a[0][3];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[2][3];
            flTerm *= m_a[1][2];
            flAcc += flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][3];
            pResult->m_a[1][3] = flAcc + flTerm;
            flTerm             = m_a[2][0];
            flTerm *= pRhs->m_a[0][0];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[2][0];
            flTerm *= m_a[2][2];
            flAcc += flTerm;
            flTerm = pRhs->m_a[1][0];
            flTerm *= m_a[2][1];
            pResult->m_a[2][0] = flAcc + flTerm;
            flTerm             = m_a[2][2];
            flTerm *= pRhs->m_a[2][1];
            flAcc  = flTerm;
            flTerm = m_a[2][1];
            flTerm *= pRhs->m_a[1][1];
            flAcc += flTerm;
            flTerm = m_a[2][0];
            flTerm *= pRhs->m_a[0][1];
            pResult->m_a[2][1] = flAcc + flTerm;
            flTerm             = m_a[2][2];
            flTerm *= pRhs->m_a[2][2];
            flAcc  = flTerm;
            flTerm = m_a[2][1];
            flTerm *= pRhs->m_a[1][2];
            flAcc += flTerm;
            flTerm = m_a[2][0];
            flTerm *= pRhs->m_a[0][2];
            pResult->m_a[2][2] = flAcc + flTerm;
            flTerm             = m_a[2][2];
            flTerm *= pRhs->m_a[2][3];
            flAcc  = flTerm;
            flTerm = m_a[2][1];
            flTerm *= pRhs->m_a[1][3];
            flAcc += flTerm;
            flTerm = m_a[2][0];
            flTerm *= pRhs->m_a[0][3];
            pResult->m_a[2][3] = flAcc + flTerm;
            flTerm             = pRhs->m_a[3][0];
            flTerm *= m_a[0][3];
            pResult->m_a[0][0] = flTerm + pResult->m_a[0][0];
            flTerm             = m_a[0][3];
            flTerm *= pRhs->m_a[3][1];
            pResult->m_a[0][1] = flTerm + pResult->m_a[0][1];
            flTerm             = m_a[0][3];
            flTerm *= pRhs->m_a[3][2];
            pResult->m_a[0][2] = flTerm + pResult->m_a[0][2];
            flTerm             = m_a[0][3];
            flTerm *= pRhs->m_a[3][3];
            pResult->m_a[0][3] = flTerm + pResult->m_a[0][3];
            flTerm             = pRhs->m_a[3][0];
            flTerm *= m_a[1][3];
            pResult->m_a[1][0] = flTerm + pResult->m_a[1][0];
            flTerm             = pRhs->m_a[3][1];
            flTerm *= m_a[1][3];
            pResult->m_a[1][1] = flTerm + pResult->m_a[1][1];
            flTerm             = pRhs->m_a[3][2];
            flTerm *= m_a[1][3];
            pResult->m_a[1][2] = flTerm + pResult->m_a[1][2];
            flTerm             = pRhs->m_a[3][3];
            flTerm *= m_a[1][3];
            pResult->m_a[1][3] = flTerm + pResult->m_a[1][3];
            flTerm             = pRhs->m_a[3][0];
            flTerm *= m_a[2][3];
            pResult->m_a[2][0] = flTerm + pResult->m_a[2][0];
            flTerm             = pRhs->m_a[3][1];
            flTerm *= m_a[2][3];
            pResult->m_a[2][1] = flTerm + pResult->m_a[2][1];
            flTerm             = pRhs->m_a[3][2];
            flTerm *= m_a[2][3];
            pResult->m_a[2][2] = flTerm + pResult->m_a[2][2];
            flTerm             = pRhs->m_a[3][3];
            flTerm *= m_a[2][3];
            pResult->m_a[2][3] = flTerm + pResult->m_a[2][3];

            pResult->m_a[3][0] = pRhs->m_a[3][0];
            pResult->m_a[3][1] = pRhs->m_a[3][1];
            pResult->m_a[3][2] = pRhs->m_a[3][2];
            pResult->m_a[3][3] = pRhs->m_a[3][3];
            break;
        }

        case 2:
        {
            flTerm = m_a[0][1];
            flTerm *= pRhs->m_a[1][0];
            flAcc  = flTerm;
            flTerm = m_a[0][2];
            flTerm *= pRhs->m_a[2][0];
            flAcc += flTerm;
            flTerm = m_a[0][0];
            flTerm *= pRhs->m_a[0][0];
            pResult->m_a[0][0] = flAcc + flTerm;
            flTerm             = m_a[0][1];
            flTerm *= pRhs->m_a[1][1];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][1];
            flTerm *= m_a[0][0];
            flAcc += flTerm;
            flTerm = pRhs->m_a[2][1];
            flTerm *= m_a[0][2];
            pResult->m_a[0][1] = flAcc + flTerm;
            flTerm             = m_a[0][1];
            flTerm *= pRhs->m_a[1][2];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][2];
            flTerm *= m_a[0][0];
            flAcc += flTerm;
            flTerm = pRhs->m_a[2][2];
            flTerm *= m_a[0][2];
            pResult->m_a[0][2] = flAcc + flTerm;
            flTerm             = m_a[0][1];
            flTerm *= pRhs->m_a[1][3];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][3];
            flTerm *= m_a[0][0];
            flAcc += flTerm;
            flTerm = pRhs->m_a[2][3];
            flTerm *= m_a[0][2];
            pResult->m_a[0][3] = flAcc + flTerm;
            flTerm             = m_a[1][2];
            flTerm *= pRhs->m_a[2][0];
            flAcc  = flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][0];
            flAcc += flTerm;
            flTerm = m_a[1][0];
            flTerm *= pRhs->m_a[0][0];
            pResult->m_a[1][0] = flAcc + flTerm;
            flTerm             = m_a[1][2];
            flTerm *= pRhs->m_a[2][1];
            flAcc  = flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][1];
            flAcc += flTerm;
            flTerm = m_a[1][0];
            flTerm *= pRhs->m_a[0][1];
            pResult->m_a[1][1] = flAcc + flTerm;
            flTerm             = m_a[1][2];
            flTerm *= pRhs->m_a[2][2];
            flAcc  = flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][2];
            flAcc += flTerm;
            flTerm = m_a[1][0];
            flTerm *= pRhs->m_a[0][2];
            pResult->m_a[1][2] = flAcc + flTerm;
            flTerm             = pRhs->m_a[2][3];
            flTerm *= m_a[1][2];
            flAcc  = flTerm;
            flTerm = m_a[1][1];
            flTerm *= pRhs->m_a[1][3];
            flAcc += flTerm;
            flTerm = m_a[1][0];
            flTerm *= pRhs->m_a[0][3];
            pResult->m_a[1][3] = flAcc + flTerm;
            flTerm             = m_a[2][2];
            flTerm *= pRhs->m_a[2][0];
            flAcc  = flTerm;
            flTerm = m_a[2][1];
            flTerm *= pRhs->m_a[1][0];
            flAcc += flTerm;
            flTerm = pRhs->m_a[0][0];
            flTerm *= m_a[2][0];
            pResult->m_a[2][0] = flAcc + flTerm;
            flTerm             = m_a[2][2];
            flTerm *= pRhs->m_a[2][1];
            flAcc  = flTerm;
            flTerm = m_a[2][1];
            flTerm *= pRhs->m_a[1][1];
            flAcc += flTerm;
            flTerm = pRhs->m_a[0][1];
            flTerm *= m_a[2][0];
            pResult->m_a[2][1] = flAcc + flTerm;
            flTerm             = m_a[2][2];
            flTerm *= pRhs->m_a[2][2];
            flAcc  = flTerm;
            flTerm = m_a[2][1];
            flTerm *= pRhs->m_a[1][2];
            flAcc += flTerm;
            flTerm = pRhs->m_a[0][2];
            flTerm *= m_a[2][0];
            pResult->m_a[2][2] = flAcc + flTerm;
            flTerm             = m_a[2][2];
            flTerm *= pRhs->m_a[2][3];
            flAcc  = flTerm;
            flTerm = m_a[2][1];
            flTerm *= pRhs->m_a[1][3];
            flAcc += flTerm;
            flTerm = pRhs->m_a[0][3];
            flTerm *= m_a[2][0];
            pResult->m_a[2][3] = flAcc + flTerm;
            flTerm             = pRhs->m_a[1][0];
            flTerm *= m_a[3][1];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[2][0];
            flTerm *= m_a[3][2];
            flAcc += flTerm;
            flTerm = m_a[3][0];
            flTerm *= pRhs->m_a[0][0];
            pResult->m_a[3][0] = flAcc + flTerm;
            flTerm             = pRhs->m_a[1][1];
            flTerm *= m_a[3][1];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][1];
            flTerm *= m_a[3][0];
            flAcc += flTerm;
            flTerm = pRhs->m_a[2][1];
            flTerm *= m_a[3][2];
            pResult->m_a[3][1] = flAcc + flTerm;
            flTerm             = pRhs->m_a[1][2];
            flTerm *= m_a[3][1];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][2];
            flTerm *= m_a[3][0];
            flAcc += flTerm;
            flTerm = pRhs->m_a[2][2];
            flTerm *= m_a[3][2];
            pResult->m_a[3][2] = flAcc + flTerm;
            flTerm             = pRhs->m_a[1][3];
            flTerm *= m_a[3][1];
            flAcc  = flTerm;
            flTerm = pRhs->m_a[0][3];
            flTerm *= m_a[3][0];
            flAcc += flTerm;
            flTerm = pRhs->m_a[2][3];
            flTerm *= m_a[3][2];
            flAcc += flTerm;
            pResult->m_a[3][3] = flAcc + m_a[3][3];

            pResult->m_a[0][3] = m_a[0][3] + pResult->m_a[0][3];
            pResult->m_a[1][3] = m_a[1][3] + pResult->m_a[1][3];
            pResult->m_a[2][3] = m_a[2][3] + pResult->m_a[2][3];
            break;
        }

        case 3:
            TransformObject::Multiply4x4(&pResult->m_a[0][0], &m_a[0][0], &pRhs->m_a[0][0]);
            break;
    }

    pResult->MarkChanged();
}

// Uses the m_rotX.flCos and m_rotX.flSin that whoever set the angle staged.
// FUNCTION: HYPERX 0x00427940
void TransformObject::RotateXStaged()
{
    if (m_bIdentity != 0)
    {
        const float flCos = m_rotX.flCos;

        m_a[2][2] = flCos;
        m_a[1][1] = flCos;
        m_a[1][2] = -m_rotX.flSin;
        m_a[2][1] = m_rotX.flSin;
    }
    else
    {
        const float flOld0 = m_a[0][1];

        m_a[0][1] = flOld0 * m_rotX.flCos + GetElement(0, 2) * m_rotX.flSin;
        m_a[0][2] = m_a[0][2] * m_rotX.flCos - flOld0 * m_rotX.flSin;

        const float flOld1 = m_a[1][1];

        m_a[1][1] = flOld1 * m_rotX.flCos + GetElement(1, 2) * m_rotX.flSin;
        m_a[1][2] = m_a[1][2] * m_rotX.flCos - flOld1 * m_rotX.flSin;

        const float flOld2 = m_a[2][1];

        m_a[2][1] = flOld2 * m_rotX.flCos + GetElement(2, 2) * m_rotX.flSin;
        m_a[2][2] = m_a[2][2] * m_rotX.flCos - flOld2 * m_rotX.flSin;

        if (m_bHasRow3 != 0)
        {
            const float flOld3 = m_a[3][1];

            m_a[3][1] = flOld3 * m_rotX.flCos + GetElement(3, 2) * m_rotX.flSin;
            m_a[3][2] = m_a[3][2] * m_rotX.flCos - flOld3 * m_rotX.flSin;
        }
    }

    MarkChanged();
}

// Uses the sine and cosine already held; an identity matrix is written, not multiplied.

// FUNCTION: HYPERX 0x00427a10
void TransformObject::ApplyRotationZ()
{
    float flFirst;

    if (m_bIdentity != 0)
    {
        flFirst = m_rotZ.flCos;

        m_a[1][1] = flFirst;
        m_a[0][0] = flFirst;
        m_a[0][1] = -m_rotZ.flSin;
        m_a[1][0] = m_rotZ.flSin;
    }
    else
    {
        flFirst = m_a[0][0];

        m_a[0][0] = flFirst * m_rotZ.flCos + GetElement(0, 1) * m_rotZ.flSin;
        m_a[0][1] = m_a[0][1] * m_rotZ.flCos - flFirst * m_rotZ.flSin;

        flFirst = m_a[1][0];

        m_a[1][0] = flFirst * m_rotZ.flCos + GetElement(1, 1) * m_rotZ.flSin;
        m_a[1][1] = m_a[1][1] * m_rotZ.flCos - flFirst * m_rotZ.flSin;

        flFirst = m_a[2][0];

        m_a[2][0] = (flFirst * m_rotZ.flCos) + m_rotZ.flSin * m_a[2][1];
        m_a[2][1] = m_rotZ.GetCos() * m_a[2][1] - flFirst * m_rotZ.flSin;

        if (m_bHasRow3 != 0)
        {
            flFirst = m_a[3][0];

            m_a[3][0] = flFirst * m_rotZ.flCos + GetElement(3, 1) * m_rotZ.flSin;
            m_a[3][1] = m_a[3][1] * m_rotZ.flCos - flFirst * m_rotZ.flSin;
        }
    }

    MarkChanged();
}

// FUNCTION: HYPERX 0x00427ae0
void TransformObject::ToTransposed()
{
    float flSwap = m_a[0][1];
    m_a[0][1]    = m_a[1][0];
    m_a[1][0]    = flSwap;

    flSwap    = m_a[0][2];
    m_a[0][2] = m_a[2][0];
    m_a[2][0] = flSwap;

    flSwap    = m_a[0][3];
    m_a[0][3] = m_a[3][0];
    m_a[3][0] = flSwap;

    flSwap    = m_a[1][2];
    m_a[1][2] = m_a[2][1];
    m_a[2][1] = flSwap;

    flSwap    = m_a[1][3];
    m_a[1][3] = m_a[3][1];
    m_a[3][1] = flSwap;

    flSwap    = m_a[2][3];
    m_a[2][3] = m_a[3][2];
    m_a[3][2] = flSwap;
}

// FUNCTION: HYPERX 0x00427b30
void TransformObject::CopyTransposed(TransformObject* pOut)
{
    pOut->m_a[0][0] = m_a[0][0];
    pOut->m_a[0][1] = m_a[1][0];
    pOut->m_a[0][2] = m_a[2][0];
    pOut->m_a[0][3] = m_a[3][0];
    pOut->m_a[1][1] = m_a[1][1];
    pOut->m_a[1][0] = m_a[0][1];
    pOut->m_a[1][2] = m_a[2][1];
    pOut->m_a[1][3] = m_a[3][1];
    pOut->m_a[2][2] = m_a[2][2];
    pOut->m_a[2][0] = m_a[0][2];
    pOut->m_a[2][1] = m_a[1][2];
    pOut->m_a[2][3] = m_a[3][2];
    pOut->m_a[3][3] = m_a[3][3];
    pOut->m_a[3][0] = m_a[0][3];
    pOut->m_a[3][1] = m_a[1][3];
    pOut->m_a[3][2] = m_a[2][3];

    pOut->m_bIdentity = 0;
}

// A singular matrix returns zero and leaves the destination alone.

// FUNCTION: HYPERX 0x00427ba0
char TransformObject::ComputeInverse(TransformObject* pDest)
{
    float flDet;

    const float flA00A11 = m_a[0][0] * m_a[1][1];
    const float flA21A00 = m_a[2][1] * m_a[0][0];
    const float flA21A10 = m_a[2][1] * m_a[1][0];
    const float flA20A01 = m_a[2][0] * m_a[0][1];
    const float flA20A11 = m_a[2][0] * m_a[1][1];

    flDet = flA00A11 * m_a[2][2] - flA21A00 * m_a[1][2] - GetElement(1, 0) * GetElement(0, 1) * m_a[2][2] + flA20A01 * m_a[1][2] + flA21A10 * m_a[0][2] - flA20A11 * m_a[0][2];

    if (flDet == 0.0)
    {
        return 0;
    }

    const float flInvDet = g_flTransformOne / flDet;

    pDest->m_a[0][0] = (m_a[2][2] * m_a[1][1] - m_a[2][1] * m_a[1][2]) * flInvDet;
    pDest->m_a[1][0] = -((m_a[2][2] * m_a[1][0] - m_a[2][0] * m_a[1][2]) * flInvDet);
    pDest->m_a[2][0] = (flA21A10 - flA20A11) / flDet;
    pDest->m_a[0][1] = -((m_a[2][2] * m_a[0][1] - m_a[2][1] * m_a[0][2]) * flInvDet);
    pDest->m_a[1][1] = (m_a[2][2] * m_a[0][0] - m_a[2][0] * m_a[0][2]) * flInvDet;
    pDest->m_a[2][1] = -((flA21A00 - flA20A01) / flDet);
    pDest->m_a[0][2] = (m_a[1][2] * m_a[0][1] - m_a[0][2] * m_a[1][1]) * flInvDet;
    pDest->m_a[1][2] = -((GetElement(0, 0) * m_a[1][2] - m_a[1][0] * m_a[0][2]) * flInvDet);
    pDest->m_a[2][2] = (flA00A11 - m_a[1][0] * m_a[0][1]) * flInvDet;

    pDest->m_a[0][3] = -(pDest->GetElement(0, 0) * m_a[0][3]) - m_a[1][3] * pDest->m_a[0][1] - pDest->GetElement(0, 2) * m_a[2][3];
    pDest->m_a[1][3] = -(pDest->m_a[1][0] * m_a[0][3]) - GetElement(1, 3) * pDest->m_a[1][1] - pDest->GetElement(1, 2) * m_a[2][3];
    pDest->m_a[2][3] = -(pDest->m_a[2][0] * m_a[0][3]) - m_a[1][3] * pDest->m_a[2][1] - pDest->GetElement(2, 2) * m_a[2][3];

    pDest->m_a[3][3] = g_flTransformOne;
    pDest->m_a[3][0] = pDest->m_a[3][1] = pDest->m_a[3][2] = g_flTransformZero;

    return 1;
}

// FUNCTION: HYPERX 0x00427d60
void __cdecl TransformObject::Multiply4x4(float* pOut, const float* pA, const float* pB)
{
    pOut[0] = pA[0] * pB[0] + pA[1] * pB[4] + pA[2] * pB[8] + pA[3] * pB[12];
    pOut[1] = pA[0] * pB[1] + pA[1] * pB[5] + pA[2] * pB[9] + pA[3] * pB[13];
    pOut[2] = pA[0] * pB[2] + pA[1] * pB[6] + pA[2] * pB[10] + pA[3] * pB[14];
    pOut[3] = pA[0] * pB[3] + pA[1] * pB[7] + pA[2] * pB[11] + pA[3] * pB[15];

    pOut[4] = pA[4] * pB[0] + pA[5] * pB[4] + pA[6] * pB[8] + pA[7] * pB[12];
    pOut[5] = pA[4] * pB[1] + pA[5] * pB[5] + pA[6] * pB[9] + pA[7] * pB[13];
    pOut[6] = pA[4] * pB[2] + pA[5] * pB[6] + pA[6] * pB[10] + pA[7] * pB[14];
    pOut[7] = pA[4] * pB[3] + pA[5] * pB[7] + pA[6] * pB[11] + pA[7] * pB[15];

    pOut[8]  = pA[8] * pB[0] + pA[9] * pB[4] + pA[10] * pB[8] + pA[11] * pB[12];
    pOut[9]  = pA[8] * pB[1] + pA[9] * pB[5] + pA[10] * pB[9] + pA[11] * pB[13];
    pOut[10] = pA[8] * pB[2] + pA[9] * pB[6] + pA[10] * pB[10] + pA[11] * pB[14];
    pOut[11] = pA[8] * pB[3] + pA[9] * pB[7] + pA[10] * pB[11] + pA[11] * pB[15];

    pOut[12] = pA[12] * pB[0] + pA[13] * pB[4] + pA[14] * pB[8] + pA[15] * pB[12];
    pOut[13] = pA[12] * pB[1] + pA[13] * pB[5] + pA[14] * pB[9] + pA[15] * pB[13];
    pOut[14] = pA[12] * pB[2] + pA[13] * pB[6] + pA[14] * pB[10] + pA[15] * pB[14];
    pOut[15] = pA[12] * pB[3] + pA[13] * pB[7] + pA[14] * pB[11] + pA[15] * pB[15];
}

// Asking for no mirror leaves the scale alone rather than restoring it.

// FUNCTION: HYPERX 0x004305f0
void TransformObject::MirrorZ(char bMirror)
{
    if (bMirror != 0)
    {
        m_vScale.x = 1.0f;
        m_vScale.y = 1.0f;
        m_vScale.z = -1.0f;

        ApplyScale();
    }
}
