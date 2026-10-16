// The camera: an engine object with four child interfaces, two smoothing rings and a surface.

#ifndef HYPERBLADE_CAMERAOBJECT_H
#define HYPERBLADE_CAMERAOBJECT_H

#include "common/compat.h"
#include "gameengine.h"
#include "engine/engineobject.h"
#include "planvm/interactor.h"

class Texture;

// The camera's two history windows, each a ring of positions or directions.
// The reticle states, as DATA/ENUMIDS.H's Reticle_State numbers them.
#define RETICLE_OFF    0
#define RETICLE_AIM    1
#define RETICLE_COMMIT 2

#define CAMERA_HISTORY_CAPACITY 25

// VTABLE: HYPERX 0x004b286c
class CameraObject : public EngineObject
{
public:
    float GetBungeeLength() const
    {
        return m_flBungeeLength;
    }

    CameraObject(int nSlot, class ObjectDef* pDef);
    void InitViewTransforms();
    ~CameraObject();

    void ViewIt();

    class EngineObject* GetTrackedObject();
    class EngineObject* GetSecondaryTrackedObject();

    // Given the arena's collision ellipsoid.
    void DrawShadowAndTransparentPass(class CollisionShape* pSurface);

    void                UpdateSmoothing();
    void                SetPosHistorySize(short nCount);
    void                SetDirHistorySize(short nCount);
    void                SetReticleState(int nState);
    void                SetSecondaryTrackedObject(class EngineObject* pObject);
    void                SetTrackedObject(class EngineObject* pObject);
    void                SetLastTrackedObject(EngineObject* pObject);
    class EngineObject* GetLastTrackedObject();
    void                UpdateDirSmoothing();
    void                SetViewTransform(unsigned int nViewport, class Texture* pSource);

    // The transparent queue is a global; each camera resizes it and the draw pass sorts it.
    void        ResizeTransparentQueue(short nHighest);
    static void SortTransparentQueue();

    // Engine slots or minus one: the two objects the camera follows, and the one tracking saw last frame.
    int m_nPrimaryTracked;   // 0x0d14
    int m_nSecondaryTracked; // 0x0d18
    int m_nLastTracked;      // 0x0d1c
    // The view builder's three axis scales.
    Vec3 m_vAxisScale; // 0x0d20
    // Points at &m_vAxisScale.x; the view builder reads the scales through it.
    Vec3* m_pSettings; // 0x0d2c
    // The bungee rest length, 12 at construction; ctrack.cpp pulls with (distance - this) * 1.5.
    float m_flBungeeLength; // 0x0d30
    // Callers clamp the window to [1, 25]; more overruns the other ring, and zero divides by zero.
    Vec3  m_aPosRing[CAMERA_HISTORY_CAPACITY]; // 0x0d34
    Vec3  m_aDirRing[CAMERA_HISTORY_CAPACITY]; // 0x0e60
    Vec3  m_posOut;                            // 0x0f8c
    Vec3  m_dirOut;                            // 0x0f98
    short m_wPosCount;                         // 0x0fa4
    short m_wPosIndex;                         // 0x0fa6
    short m_wDirCount;                         // 0x0fa8
    short m_wDirIndex;                         // 0x0faa
    // One over the sample count, so the ring's sum becomes its mean.
    float m_flPosWeight; // 0x0fac
    float m_flDirWeight; // 0x0fb0

    // Float parameters, so a stored constant passes through the FPU as in the client.
    void SetViewFracX(float flFrac)
    {
        m_flViewFracX = flFrac;
    }

    void SetViewFracY(float flFrac)
    {
        m_flViewFracY = flFrac;
    }

    float GetViewFracX() const
    {
        return m_flViewFracX;
    }

    float GetViewFracY() const
    {
        return m_flViewFracY;
    }

    float m_flViewFracX;   // 0x0fb4
    float m_flViewFracY;   // 0x0fb8
    int   m_nReticleState; // 0x0fbc
    // The helmet camera's shot power meter, 100 at construction.
    int m_nShotPowerLevel; // 0x0fc0
    // Whether the meter is shown; id_ACF_SetShotPowerMeter sets it.
    char m_bShotPowerShown; // 0x0fc4
    // The surface the camera draws into.
    Texture* m_pTarget; // 0x0fc8
    // Handed to D3DViewport::SetForTexture as the viewport this camera draws into.
    int         nViewportId; // 0x0fcc
    LayoutState layout;      // 0x0fd0
    // Half the field of view in radians: half of 85 degrees at construction.
    float m_flHalfFov; // 0x1010
    // A quarter of a degree in radians, set by the constructor. Nothing reads it.
    float m_flUnused; // 0x1014
    // Zeroed by the constructor and read nowhere.
    int m_nUnusedD; // 0x1018
    // The first two are CameraProjectionState and the last two TransformObject.
    class CameraProjectionState* m_pProjection; // 0x101c
    class CameraProjectionState* m_pView;       // 0x1020
    TransformObject*             m_pRotB;       // 0x1024
    TransformObject*             m_pRotA;       // 0x1028
    // Only the constructor sets it.
    char m_bRearView; // 0x102c
};

// SYNTHETIC: HYPERX 0x00421c10
// CameraObject::`scalar deleting destructor'

static_assert(sizeof(CameraObject) == 0x1030);

inline CameraObject* GameEngine::GetCamera(int nSlot) const
{
    return (CameraObject*)m_ppSlots[nSlot];
}

inline CameraObject* Interactor::GetCameraActor()
{
    return (CameraObject*)GetActor();
}

#endif
