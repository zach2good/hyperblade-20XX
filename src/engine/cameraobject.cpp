#include "common/compat.h"
#include "cameraobject.h"
#include "anim/animbinding.h"
#include "devicector.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "objectclasses.h"
#include "physics/physicsbody.h"
#include "common/staticinit.h"
#include "graphics/transparentqueue.h"
#include "math/vector.h"
#include "gameengine.h"
#include "cameraprojectionstate.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "math/mathglobals.h"
#include "math/transformobject.h"
#include "clipsphere.h"
#include "graphics/texture.h"
#include "graphics/meshbone.h"
#include <string.h>
#include <math.h>
#include "anim/animplayer.h"
#include "app/gameapp.h"
#include "ui/widget.h"
#include "physics/collisionplane.h"
#include "engine/engineobject.h"
#include "math/point.h"
#include "graphics/d3drend.h"

extern const float g_flZeroCamera    = 0.0f;
float              g_flUnusedCameraA = g_flZeroCamera;
float              g_flUnusedCameraB = g_flZeroCamera;
float              g_flUnusedCameraC = g_flZeroCamera;

// The camera object: construction, smoothing rings, view pass, view transforms and tracked slots.

// One, defined with its marker further down this file.
extern const float g_flOneCamera;

// A half, copied into both viewport dimensions.
// GLOBAL: HYPERX 0x004b2854
extern const float g_flHalfCamera;

// Two, defined further down this file beside the camera's own pi.
extern const float g_flTwoCamera;

// Both rings are seeded with the entity's position and a look a hundred behind the origin.

// FUNCTION: HYPERX 0x00421980
CameraObject::CameraObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    Vec3 vDir;

    m_pAnimPlayer->SetObject(this);

    m_nSecondaryTracked = -1;
    m_nPrimaryTracked   = -1;
    m_nLastTracked      = -1;

    m_vAxisScale.x = m_vAxisScale.y = m_vAxisScale.z = g_flZeroCamera;
    m_flBungeeLength                                 = 12.0f;
    m_pSettings                                      = &m_vAxisScale;

    m_flViewFracX     = g_flHalfCamera;
    m_flViewFracY     = g_flHalfCamera;
    m_nReticleState   = RETICLE_OFF;
    m_bShotPowerShown = 0;
    m_nShotPowerLevel = 100;
    m_wPosCount       = 6;
    m_wPosIndex       = 0;
    m_wDirCount       = 4;
    m_wDirIndex       = 0;

    m_flPosWeight = g_flOneCamera * 0.16666667f;
    m_flDirWeight = g_flOneCamera * 0.25f;

    PhysicsBody* pBody = m_pBody;

    vDir.x = g_flZeroCamera - pBody->pos.x;
    vDir.y = g_flZeroCamera - pBody->pos.y;
    vDir.z = g_flCameraStartLookZ - pBody->pos.z;

    Math::Vec3Normalize(&vDir);

    for (int i = 0; i < CAMERA_HISTORY_CAPACITY; i++)
    {
        m_aPosRing[i].x = m_pBody->pos.x;
        m_aPosRing[i].y = m_pBody->pos.y;
        m_aPosRing[i].z = m_pBody->pos.z;
        m_aDirRing[i].x = vDir.x;
        m_aDirRing[i].y = vDir.y;
        m_aDirRing[i].z = vDir.z;
    }

    pBody = m_pBody;

    m_posOut.x = pBody->pos.x;
    m_posOut.y = pBody->pos.y;
    m_posOut.z = pBody->pos.z;
    m_dirOut.x = vDir.x;
    m_dirOut.y = vDir.y;
    m_dirOut.z = vDir.z;

    pBody->velDir.x   = vDir.x;
    m_pBody->velDir.y = m_dirOut.y;
    m_pBody->velDir.z = m_dirOut.z;

    m_pTarget   = 0;
    nViewportId = 0;

    memset(&layout, 0, sizeof(layout));

    m_flHalfFov = g_flDegToRadCamera * 85.0f / g_flTwoCamera;

    m_flUnused = g_flDegToRadCamera * 0.25f;

    m_nUnusedD    = 0;
    m_pProjection = 0;
    m_pView       = 0;
    m_pRotB       = 0;
    m_pRotA       = 0;

    g_pTransparentQueue    = 0;
    g_nTransparentCapacity = 0;
    g_nTransparentCount    = 0;

    m_bRearView = 0;
}

// FUNCTION: HYPERX 0x00421c30
void CameraObject::ResizeTransparentQueue(short nHighest)
{
    if (g_nTransparentCapacity != nHighest + 1)
    {
        if (g_pTransparentQueue != 0)
        {
            delete[] g_pTransparentQueue;
        }

        g_nTransparentCapacity = (short)(nHighest + 1);

        g_pTransparentQueue = new TransparentEntry[g_nTransparentCapacity];

        memset(g_pTransparentQueue, 0, g_nTransparentCapacity * sizeof(TransparentEntry));
    }
}

// Each release's answer is stored back over the pointer it released.

// FUNCTION: HYPERX 0x00421ca0
CameraObject::~CameraObject()
{
    m_nSecondaryTracked = -1;
    m_nPrimaryTracked   = -1;
    m_nLastTracked      = -1;

    if (m_pProjection != 0)
    {
        m_pProjection = (CameraProjectionState*)m_pProjection->DestroyViaVtable();
    }

    if (m_pView != 0)
    {
        m_pView = (CameraProjectionState*)m_pView->DestroyViaVtable();
    }

    if (m_pRotB != 0)
    {
        m_pRotB = m_pRotB->DestroyViaVtable();
    }

    if (m_pRotA != 0)
    {
        m_pRotA = m_pRotA->DestroyViaVtable();
    }

    g_nTransparentCount    = 0;
    g_nTransparentCapacity = 0;

    if (g_pTransparentQueue != 0)
    {
        delete[] g_pTransparentQueue;

        g_pTransparentQueue = 0;
    }
}

// FUNCTION: HYPERX 0x00421d80
void CameraObject::UpdateSmoothing()
{
    PhysicsBody* const pSource = m_pBody;

    Vec3 vPosition = pSource->pos;

    const int iPosition = m_wPosIndex;

    m_posOut.x += (vPosition.x - m_aPosRing[iPosition].x) * m_flPosWeight;
    m_posOut.y += (vPosition.y - m_aPosRing[iPosition].y) * m_flPosWeight;
    m_posOut.z += (vPosition.z - m_aPosRing[iPosition].z) * m_flPosWeight;

    const int iLookAt = m_wDirIndex;

    m_dirOut.x += (pSource->velDir.x - m_aDirRing[iLookAt].x) * m_flDirWeight;
    m_dirOut.y += (pSource->velDir.y - m_aDirRing[iLookAt].y) * m_flDirWeight;
    m_dirOut.z += (pSource->velDir.z - m_aDirRing[iLookAt].z) * m_flDirWeight;

    m_aPosRing[iPosition] = pSource->pos;

    m_aDirRing[m_wDirIndex] = m_pBody->velDir;

    m_wPosIndex = (short)((m_wPosIndex + 1) % m_wPosCount);

    m_wDirIndex = (short)((m_wDirIndex + 1) % m_wDirCount);
}

// Answers nothing; its one caller ignores EAX.

// FUNCTION: HYPERX 0x00421ef0
void CameraObject::UpdateDirSmoothing()
{
    const int nIndex = m_wDirIndex;

    Vec3* const pLookAt = &m_pBody->velDir;

    m_dirOut.x += (pLookAt->x - m_aDirRing[nIndex].x) * m_flDirWeight;
    m_dirOut.y += (pLookAt->y - m_aDirRing[m_wDirIndex].y) * m_flDirWeight;
    m_dirOut.z += (pLookAt->z - m_aDirRing[nIndex].z) * m_flDirWeight;

    m_aDirRing[nIndex] = *pLookAt;

    const int nNext = m_wDirIndex + 1;

    m_wDirIndex = (short)(nNext % m_wDirCount);
}

// Three, the shake vector's z.
// GLOBAL: HYPERX 0x004bc3d8
extern float g_flCameraShakeZ;

// The shake vector's roll comes from the position history; flag bit four hides an object.

// FUNCTION: HYPERX 0x00421f90
void CameraObject::ViewIt()
{
    Vec3       vRight;
    Vec3       vCentre;
    Vec3       vUp;
    Vec3       vCross;
    ClipSphere clip;

    if (g_pScene != this)
    {
        D3DRend::FlushBatch(0);

        g_pScene = this;

        Renderer::SetCameraTransformAndViewport(layout);
    }

    if (g_pRenderer != 0 && m_nSlot == CAMERA_HEAD_SLOT)
    {
        g_pRenderer->ShowHeadCam();
    }

    EngineObject* const pTracked = GetTrackedObject();

    if (pTracked == 0)
    {
        // STRING: HYPERX 0x004bc3dc
        g_gameApp.Fatal("Camera::ViewIt(): Null pointer for ModelToTrack\n");
    }

    const short nIndex = m_wPosIndex;

    const float flShakeX = m_aPosRing[nIndex].x * -0.01f;
    const float flShakeY = m_aPosRing[nIndex].y * -0.001f;
    const float flShakeZ = g_flCameraShakeZ;

    const short nCount = g_pGameEngine->m_nObjectCount;

    m_pRotB->m_a[0][1] = m_dirOut.x;
    m_pRotB->m_a[1][1] = m_dirOut.y;
    m_pRotB->m_a[2][1] = m_dirOut.z;
    m_pRotB->m_a[3][1] = g_flZeroCamera;

    vRight.x = flShakeZ * m_dirOut.y - flShakeY * m_dirOut.z;
    vRight.y = flShakeX * m_dirOut.z - flShakeZ * m_dirOut.x;
    vRight.z = flShakeY * m_dirOut.x - flShakeX * m_dirOut.y;

    Math::Vec3Normalize(&vRight);

    m_pRotB->m_a[0][0] = vRight.x;
    m_pRotB->m_a[1][0] = vRight.y;
    m_pRotB->m_a[2][0] = vRight.z;
    m_pRotB->m_a[3][0] = g_flZeroCamera;

    vUp.x = vRight.y * m_dirOut.z - vRight.z * m_dirOut.y;
    vUp.y = vRight.z * m_dirOut.x - vRight.x * m_dirOut.z;
    vUp.z = vRight.x * m_dirOut.y - vRight.y * m_dirOut.x;

    Math::Vec3Normalize(&vUp);

    m_pRotB->m_a[0][2] = vUp.x;
    m_pRotB->m_a[1][2] = vUp.y;
    m_pRotB->m_a[2][2] = vUp.z;
    m_pRotB->m_a[3][2] = g_flZeroCamera;
    m_pRotB->m_a[2][3] = g_flZeroCamera;
    m_pRotB->m_a[1][3] = g_flZeroCamera;
    m_pRotB->m_a[0][3] = g_flZeroCamera;
    m_pRotB->m_a[3][3] = g_flOneCamera;

    m_pRotB->ToTransposed();

    m_pProjection->Multiply(m_pView,
                            m_pRotB);

    Vec3 vDir = m_dirOut;

    Vec3 vNormal = pTracked->GetPhysicsBody()->surfNormal;

    vCross.x = vDir.y * vNormal.z - vDir.z * vNormal.GetY();
    vCross.y = vDir.z * vNormal.x - vDir.x * vNormal.z;
    vCross.z = vDir.x * vNormal.y - vDir.y * vNormal.x;

    vCross.ScaleInPlace(m_pSettings->x);
    vDir.ScaleInPlace(m_pSettings->y);
    vNormal.ScaleInPlace(m_pSettings->z);

    Vec3 vPos = m_posOut;

    vPos.x = (vPos.x + vCross.x) + vDir.x;
    vPos.y = vPos.y + vCross.y + vDir.y;
    vPos.z = (vPos.z + vNormal.z) + vCross.z + vDir.z;

    if (g_hudSurfaces.bHud320 == 0 || m_nSlot != CAMERA_HEAD_SLOT)
    {
        m_pView->SetEyePoint(&vPos);

        g_nTransparentCount = 0;

        for (short i = 0; i < nCount; i++)
        {
            EngineObject* const pObject = g_pGameEngine->m_ppObjects[i];

            pObject->ComputeWorldBoundingCentre(&vCentre);

            if (((unsigned char)pObject->m_nFlags & (ENGINEOBJECT_HIDDEN | ENGINEOBJECT_POSED)) == 0)
            {
                pObject->ApplyRenderSlotPose();
            }

            if (pObject->IsHidden() == 0)
            {
                if (((unsigned char)pObject->m_nFlags & ENGINEOBJECT_NO_CULL) == 0)
                {
                    const TransformObject* pView    = m_pView;
                    const float            flRadius = pObject->m_flBoundRadius;

                    clip.x = (((vCentre.x * pView->m_a[0][0]) + vCentre.y * pView->m_a[0][1]) + vCentre.z * pView->m_a[0][2]) + pView->m_a[0][3];

                    clip.y = (((vCentre.x * pView->m_a[1][0]) + vCentre.y * pView->m_a[1][1]) + vCentre.z * pView->m_a[1][2]) + pView->m_a[1][3];
                    clip.z = (((vCentre.x * pView->m_a[2][0]) + vCentre.y * pView->m_a[2][1]) + vCentre.z * pView->m_a[2][2]) + pView->m_a[2][3];

                    if (pView->m_bHasRow3 != 0)
                    {
                        clip.w = (((vCentre.x * pView->m_a[3][0]) + vCentre.y * pView->m_a[3][1]) + vCentre.z * pView->m_a[3][2]) + pView->m_a[3][3];
                    }
                    else
                    {
                        clip.w = g_flOneCamera;
                    }

                    if (clip.FrustumCull(flRadius) == 0)
                    {
                        pObject->SetVisible(0);

                        continue;
                    }
                }

                pObject->SetVisible(1);
                pObject->UpdateLODAndPushComponents(this);
            }
            else
            {
                pObject->SetVisible(0);
            }
        }

        Renderer::DrawSceneDispatch();
    }
}

// One, the numerator of the boxcar weight.
// GLOBAL: HYPERX 0x004b284c
extern const float g_flOneCamera;

// FUNCTION: HYPERX 0x00422450
void CameraObject::SetReticleState(int nState)
{
    m_nReticleState = nState;

    if (nState == RETICLE_AIM)
    {
        m_nShotPowerLevel = 0;
    }

    if (g_pRenderer != 0)
    {
        g_pRenderer->SetReticleState(nState);

        if (nState == RETICLE_AIM)
        {
            g_pRenderer->SetShotPower(0);
        }
    }
}

// A count of 0 divides by 0; only the callers' clamp to [1, 25] prevents it.
// FUNCTION: HYPERX 0x00422490
void CameraObject::SetPosHistorySize(short nCount)
{
    Vec3 vSum;

    vSum.x = g_flZeroCamera;
    vSum.y = g_flZeroCamera;
    vSum.z = g_flZeroCamera;

    short nOld    = m_wPosCount;
    m_wPosCount   = nCount;
    m_flPosWeight = g_flOneCamera / nCount;

    if (m_wPosIndex >= nCount)
    {
        m_wPosIndex = 0;
    }

    while (nOld < m_wPosCount)
    {
        m_aPosRing[nOld] = m_posOut;
        nOld++;
    }

    for (short i = 0; i < m_wPosCount; i++)
    {
        vSum.x += m_aPosRing[i].x;
        vSum.y += m_aPosRing[i].y;
        vSum.z += m_aPosRing[i].z;
    }

    vSum.x *= m_flPosWeight;
    vSum.y *= m_flPosWeight;
    vSum.z *= m_flPosWeight;

    m_posOut.x = vSum.x;
    m_posOut.y = vSum.y;
    m_posOut.z = vSum.z;
}

// FUNCTION: HYPERX 0x004225a0
void CameraObject::SetDirHistorySize(short nCount)
{
    Vec3 vSum;

    vSum.x = g_flZeroCamera;
    vSum.y = g_flZeroCamera;
    vSum.z = g_flZeroCamera;

    short nOld    = m_wDirCount;
    m_wDirCount   = nCount;
    m_flDirWeight = g_flOneCamera / nCount;

    if (m_wDirIndex >= nCount)
    {
        m_wDirIndex = 0;
    }

    while (nOld < m_wDirCount)
    {
        m_aDirRing[nOld] = m_dirOut;
        nOld++;
    }

    for (short i = 0; i < m_wDirCount; i++)
    {
        vSum.x += m_aDirRing[i].x;
        vSum.y += m_aDirRing[i].y;
        vSum.z += m_aDirRing[i].z;
    }

    vSum.x *= m_flDirWeight;
    vSum.y *= m_flDirWeight;
    vSum.z *= m_flDirWeight;

    Vec3* const pOut = &m_dirOut;
    pOut->x          = vSum.x;
    pOut->y          = vSum.y;
    pOut->z          = vSum.z;
}

// The view is the extent less twice the border, rounded up to a multiple of four in width.

// FUNCTION: HYPERX 0x004226b0
void CameraObject::SetViewTransform(unsigned int nViewport, Texture* pSource)
{
    LayoutState state;

    nViewportId = nViewport;
    m_pTarget   = pSource;

    Point extent = pSource->GetSize();
    Point border = m_pTarget->GetInset();

    const int nWidth  = (extent.x - border.x * 2 + 3) & ~3;
    const int nHeight = extent.y - border.y * 2;

    // The pitch rounds the width again, which does nothing.
    state = Widget::InitLayoutState(0, nWidth, nHeight, (nWidth + 3) & ~3, nWidth, nHeight, 0, 0);

    layout = state;
}

// Two, beside the camera's own pi.
// GLOBAL: HYPERX 0x004b2850
extern const float g_flTwoCamera;

// The two transforms built on the stack are constructed, destroyed and never read.
// FUNCTION: HYPERX 0x00422750
void CameraObject::InitViewTransforms()
{
    if (m_pView != 0)
    {
        m_pView->ResetProjection(&layout, m_flHalfFov);
    }
    else
    {
        m_pView = new CameraProjectionState(1, &layout, m_flHalfFov);
    }

    layout.pCamera = m_pView;

    if (m_pProjection != 0)
    {
        m_pProjection->ResetProjection(&layout, m_flHalfFov);
    }
    else
    {
        m_pProjection = new CameraProjectionState(1, &layout, m_flHalfFov);
    }

    if (m_pRotA != 0)
    {
        m_pRotA->ResetToIdentity();
    }
    else
    {
        m_pRotA = new TransformObject(1);
    }

    if (m_pRotB != 0)
    {
        m_pRotB->ResetToIdentity();
    }
    else
    {
        m_pRotB = new TransformObject(1);
    }

    TransformObject tScratchB;
    TransformObject tScratchA;

    const float flTilt = g_flPiCamera / g_flTwoCamera;

    if (g_bBottomUpDIB != 0)
    {
        m_pRotA->RotateX(-flTilt);

        m_pProjection->MirrorZ(g_bBottomUpDIB);
    }
    else
    {
        m_pRotA->RotateX(flTilt);
    }

    m_pProjection->Compose(m_pRotA);
}

// The Numerical Recipes heapsort, one-based, so element zero is never touched.
// FUNCTION: HYPERX 0x00422a30
void CameraObject::SortTransparentQueue()
{
    if (g_nTransparentCount < 2)
    {
        return;
    }

    int ir = g_nTransparentCount;
    int l  = (ir >> 1) + 1;

    for (;;)
    {
        if (l > 1)
        {
            l--;

            g_transparentSiftEntry = g_pTransparentQueue[l];
        }
        else
        {
            g_transparentSiftEntry = g_pTransparentQueue[ir];

            g_pTransparentQueue[ir] = g_pTransparentQueue[1];

            ir--;

            if (ir == 1)
            {
                g_pTransparentQueue[1] = g_transparentSiftEntry;

                return;
            }
        }

        int i = l;
        int j = l + l;

        while (j <= ir)
        {
            if (j < ir && g_pTransparentQueue[j].flSortZ < g_pTransparentQueue[j + 1].flSortZ)
            {
                j++;
            }

            if (g_transparentSiftEntry.flSortZ < g_pTransparentQueue[j].flSortZ)
            {
                g_pTransparentQueue[i] = g_pTransparentQueue[j];

                i = j;
                j += j;
            }
            else
            {
                j = ir + 1;
            }
        }

        g_pTransparentQueue[i] = g_transparentSiftEntry;
    }
}

// The transparent queue is one-based and walked back to front.
// FUNCTION: HYPERX 0x00422ba0
void CameraObject::DrawShadowAndTransparentPass(CollisionShape* pSurface)
{
    const Vec3* pLightDir;
    short       nIndex;

    SortTransparentQueue();

    for (nIndex = 0; nIndex < g_pGameEngine->m_nObjectCount; nIndex++)
    {
        EngineObject* pObject = g_pGameEngine->m_ppObjects[nIndex];

        if (pObject->GetShadowCount() != 0 && pObject->IsVisible() != 0 && pObject->GetSlot() != GOALIE_SLOT_A && pObject->GetSlot() != GOALIE_SLOT_B)
        {
            pObject->DrawTransformedMesh(pSurface);
        }
    }

    TransparentEntry* pEntry = g_pTransparentQueue + g_nTransparentCount;

    for (nIndex = 0; nIndex < g_nTransparentCount; nIndex++)
    {
        if (pEntry->flWorldY > 0.0f)
        {
            pLightDir = &g_lightDirs[1].dir;
        }
        else
        {
            pLightDir = &g_lightDirs[2].dir;
        }

        const Vec3 vDir    = *pLightDir;
        g_lightDirs[0].dir = vDir;

        MeshBone* const pSlot = pEntry->pComponent;

        pEntry--;

        pSlot->DrawFaces();
    }
}

// The slot is tested as a dword and read as a word, so one above 32767 indexes backwards.

// FUNCTION: HYPERX 0x00422c70
EngineObject* CameraObject::GetSecondaryTrackedObject()
{
    const int nSlot = m_nSecondaryTracked;

    if (nSlot != -1)
    {
        return g_pGameEngine->m_ppSlots[(short)nSlot];
    }

    return 0;
}

// FUNCTION: HYPERX 0x00422c90
void CameraObject::SetSecondaryTrackedObject(EngineObject* pObject)
{
    if (pObject == 0)
    {
        m_nSecondaryTracked = -1;

        return;
    }

    m_nSecondaryTracked = pObject->GetSlot();
}

// FUNCTION: HYPERX 0x00422cc0
EngineObject* CameraObject::GetLastTrackedObject()
{
    if (m_nLastTracked == -1)
    {
        return 0;
    }

    return g_pGameEngine->m_ppSlots[(short)m_nLastTracked];
}

// A tracked object that goes away leaves a stale index; nothing tracked is minus one.
// FUNCTION: HYPERX 0x00422ce0
void CameraObject::SetLastTrackedObject(EngineObject* pObject)
{
    if (pObject == 0)
    {
        m_nLastTracked = -1;

        return;
    }

    m_nLastTracked = pObject->GetSlot();
}

// FUNCTION: HYPERX 0x00422d10
EngineObject* CameraObject::GetTrackedObject()
{
    if (m_nPrimaryTracked == -1)
    {
        return 0;
    }

    return g_pGameEngine->m_ppSlots[(short)m_nPrimaryTracked];
}

// FUNCTION: HYPERX 0x00422d30
void CameraObject::SetTrackedObject(EngineObject* pObject)
{
    if (pObject == 0)
    {
        m_nPrimaryTracked = -1;

        return;
    }

    m_nPrimaryTracked = pObject->GetSlot();
}

float g_flCameraShakeZ = 3.0f;
