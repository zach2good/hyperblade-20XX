// The camera's projection: a transform with 27 slots and 0x9c bytes of projection state.

#ifndef HYPERBLADE_CAMERAPROJECTIONSTATE_H
#define HYPERBLADE_CAMERAPROJECTIONSTATE_H

#include "math/transformobject.h"
#include "graphics/projrender.h"
#include "common/staticassert.h"

// Allocated with new, so the base's constructor carries an unwind state.
// VTABLE: HYPERX 0x004b28f4
class CameraProjectionState : public TransformObject
{
public:
    CameraProjectionState(char cFlag, struct LayoutState* pViewport, float flFov)
    {
        CreateProjectionState(cFlag, pViewport, flFov);
    }

    virtual void InitializeProjection(char cFlag, struct LayoutState* pViewport, float flFov);
    // cameraobject.cpp passes its own LayoutState and the field of view.
    virtual void ResetProjection(struct LayoutState* pViewport, float flFov);
    virtual void CreateProjectionState(char cFlag, struct LayoutState* pViewport, float flFov);
    // Slot eighteen: fills m_flTanHalfFov, m_flFarClip and m_flNearClip.
    virtual void Configure(struct LayoutState* pView, float flHalfFov, float flFarClip);
    virtual void BuildMatrix();
    // Slot twenty. cameraobject.cpp calls it with one pointer argument.
    virtual void SetEyePoint(const Vec3* pEye);

    // Slots twenty one and twenty two; nothing in this tree calls either.
    virtual void  GetEyePoint(Vec3* pOut);
    virtual float GetHalfFov();
    // Slots 23 to 26, this class's own bodies rather than the transform's.
    virtual void RotateY(float flAngle);
    virtual void RotateX(float flAngle);
    virtual void RotateZ(float flAngle);
    virtual void SubtractTranslation(float flX, float flY, float flZ);

    // 0x75 is m_bHasRow3 on the transform and the perspective flag here; which is unknown.
    float m_flCmPerPixel;    // 0x7c
    float m_flPixelsPerCm;   // 0x80
    float m_flHalfWidthPxA;  // 0x84
    float m_flHalfHeightPxA; // 0x88
    float m_flHalfWidthPxB;  // 0x8c
    float m_flHalfHeightPxB; // 0x90
    float m_flHalfWidthPx;   // 0x94
    float m_flHalfHeightPx;  // 0x98
    float m_flHalfHeightCm;  // 0x9c
    float m_flHalfWidthCm;   // 0xa0
    float m_flOffsetXPx;     // 0xa4
    float m_flOffsetYPx;     // 0xa8
    // The half field of view and its three trigonometric values.
    float  m_flHalfFov;    // 0xac
    double m_dTanHalfFov;  // 0xb0
    double m_dCosHalfFov;  // 0xb8
    double m_dSinHalfFov;  // 0xc0
    float  m_flTanHalfFov; // 0xc8
    float  m_flFarClip;    // 0xcc
    float  m_flNearClip;   // 0xd0
    // Where the eye sits; SetEyePoint negates it into m_vTranslate.
    Vec3  m_vEye;          // 0xd4
    float m_flCotX;        // 0xe0
    float m_flCotY;        // 0xe4
    float m_flTanX;        // 0xe8
    float m_flTanY;        // 0xec
    float m_flTanXSquared; // 0xf0
    float m_flTanYSquared; // 0xf4

private:
    char m_cUnusedA; // 0xf8
    // CreateProjectionState clears it and nothing reads it back.
    char m_cUnusedB; // 0xf9

public:
    // The software rasteriser's clip rectangle, copied into g_projRenderParams.
    ClipExtents m_clip; // 0xfc

private:
    static void StoreTranslate(CameraProjectionState* pState, float flX, float flY, float flZ);
};

// SYNTHETIC: HYPERX 0x00422de0
// CameraProjectionState::~CameraProjectionState

// SYNTHETIC: HYPERX 0x00422dc0
// CameraProjectionState::`scalar deleting destructor'

STATIC_ASSERT(sizeof(CameraProjectionState) == 0x118);
#endif
