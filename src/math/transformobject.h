#ifndef HYPERBLADE_TRANSFORMOBJECT_H
#define HYPERBLADE_TRANSFORMOBJECT_H

struct Vec3;

#include "vector.h"
#include "common/staticassert.h"
#include <math.h>

#define SERIAL_LAST 0xffffffff

// One axis's staged rotation, copied as a pair.
struct SinCos
{
    float GetCos() const
    {
        return flCos;
    }

    float flCos;
    float flSin;
};

// One matrix row: three axis components, then the translation.
union MatrixRow
{
    float af[4];

    struct
    {
        Vec3  vAxis;
        float flTranslation;
    };
};

// Fifteen slots. src/graphics/meshbone.cpp writes this table into a static by hand.
// VTABLE: HYPERX 0x004b223c
class TransformObject
{
public:
    static void __cdecl Multiply4x4(float* pOut, const float* pA, const float* pB);

public:
    // The matrix is no longer identity, and its serial steps on, wrapping to zero after the last.
    void MarkChanged()
    {
        m_bIdentity = 0;

        if (m_nSerial++ >= SERIAL_LAST)
        {
            m_nSerial = 0;
        }
    }

    virtual ~TransformObject();

    // The linker folded this onto PooledOwnerRoot::Destroy, which carries the marker.
    virtual TransformObject* DestroyViaVtable()
    {
        delete this;

        return 0;
    }

    virtual void       Reset(char bHasRow3);
    virtual void       ResetToIdentity();
    virtual MatrixRow* GetMatrixPtr();
    virtual void       ApplyScaleToTarget(float flScaleX, float flScaleY, float flScaleZ, TransformObject* pDest);
    virtual void       ApplyTranslation();
    virtual Vec3*      GetTranslationPtr();
    virtual void       SetTranslation(float flX, float flY, float flZ);
    virtual void       AddTranslationVec(const Vec3* pDelta);
    virtual void       AddTranslation(float flX, float flY, float flZ);
    virtual void       ToTransposed();
    virtual void       CopyTransposed(TransformObject* pOut);
    virtual char       ComputeInverse(TransformObject* pDest);
    virtual Vec3*      GetScaleVec(Vec3* pOut);

    TransformObject();

    // Only the camera's two rotation transforms are built with a fourth row.
    explicit TransformObject(char bHasRow3);

    void Compose(const TransformObject* pOther);

    // The whole product into pResult, in the original's summation order.
    void Multiply(TransformObject* pResult, const TransformObject* pRhs) const;

    // Applies the sine and cosine the object already holds.
    void RotateAroundY();

    void RotateX(float flAngle)
    {
        m_rotX.flCos = (float)cos(flAngle);
        m_rotX.flSin = (float)sin(flAngle);

        RotateXStaged();
    }

    // Applies the cos/sin pair in m_rotZ.
    void ApplyRotationZ();

    // The three rotation columns, the last first, without the translation.
    inline void CopyRotation(const TransformObject* pSource)
    {
        m_a[0][2] = pSource->m_a[0][2];
        m_a[1][2] = pSource->m_a[1][2];
        m_a[2][2] = pSource->m_a[2][2];
        m_a[0][1] = pSource->m_a[0][1];
        m_a[1][1] = pSource->m_a[1][1];
        m_a[2][1] = pSource->m_a[2][1];
        m_a[0][0] = pSource->m_a[0][0];
        m_a[1][0] = pSource->m_a[1][0];
        m_a[2][0] = pSource->m_a[2][0];
    }

    void RotateXStaged();

    float GetElement(int nRow, int nCol) const
    {
        return m_a[nRow][nCol];
    }

    float GetScaleX() const
    {
        return m_vScale.x;
    }

    float GetScaleY() const
    {
        return m_vScale.y;
    }

    float GetScaleZ() const
    {
        return m_vScale.z;
    }

    float GetTranslateX() const
    {
        return m_vTranslate.x;
    }

    void TransformPoint(Vec3 vPoint, Vec3* pOut);
    void TransformDirectionByRows(Vec3 vDir, Vec3* pOut);
    // TransformPoint without the translation term.
    void TransformDirection(Vec3 vec, Vec3* pOut);

    void ApplyScale();
    void MirrorZ(char bMirror);

    // The mesh bone holds two of these at 0x54 and 0xe0; the 16 bytes between belong to it.
    // 0x04, row major; column 3 is the translation and the bottom row (0, 0, 0, 1).
    union
    {
        float     m_a[4][4];
        MatrixRow m_aMatrixRows[4];
    };

    Vec3 m_vScale; // 0x44
    // A second copy of the translation, written by the eye setter and the shadow bind.
    Vec3 m_vTranslate; // 0x50
    // The staged sine and cosine pairs the rotation helpers read.
    SinCos       m_rotY;      // 0x5c
    SinCos       m_rotX;      // 0x64
    SinCos       m_rotZ;      // 0x6c
    char         m_bIdentity; // 0x74
    char         m_bHasRow3;  // 0x75
    unsigned int m_nSerial;   // 0x78
};

// SYNTHETIC: HYPERX 0x00416f10
// TransformObject::`scalar deleting destructor'

inline TransformObject::TransformObject()
{
    Reset(0);
}

inline TransformObject::TransformObject(char bHasRow3)
{
    Reset(bHasRow3);
}

STATIC_ASSERT(sizeof(TransformObject) == 0x7c);
#endif
