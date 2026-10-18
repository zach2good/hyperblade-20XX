#include "common/compat.h"
#include <cstdint>
#include "graphics/hwrenderer.h"
#include "graphics/graphicssettings.h"
#include "graphics/hwmodels.h"
#include <vector>
#include "engine/messageids.h"
#include "engine/planbehaviourword.h"
#include "planvm/gamebehaviorobject.h"
#include "objectclasses.h"
#include "teamsblock.h"
#include "devicector.h"
#include "engine/objectstats.h"
#include "globals.h"
#include "gameengine.h"
#include "physics/physicsbody.h"
#include "math/vector.h"
#include "planvm/planvm.h"
#include "objectdef.h"
#include "anim/animbinding.h"
#include "math/transformobject.h"
#include "graphics/meshbone.h"
#include "common/staticinit.h"
#include "physics/boundbubble.h"
#include "graphics/lodnode.h"
#include "graphics/cd3dtexturemanager.h"
#include "graphics/transformedvertex.h"
#include "graphics/modeldatabase.h"
#include "graphics/colorpalette.h"
#include "common/pooledclasses.h"
#include "net/session.h"
#include "cameraprojectionstate.h"
#include "cameraobject.h"
#include "graphics/meshmaterial.h"
#include "sound/adliberror.h"
#include "physics/collisioncage.h"
#include "planvm/agendaitem.h"
#include "statids.h"
#include "playerobject.h"
#include "anim/animplayer.h"
#include "app/gameapp.h"
#include "net/cplayer.h"
#include "planvm/msginteractor.h"
#include "rokobject.h"
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <new>
#include <cstdio>
#include "physics/collisionplane.h"
#include "engine/engineobject.h"
#include "net/netrecords.h"
#include "net/netsequenceword.h"
#include "net/slotnibbles.h"
#include "planvm/damagegate.h"
#include "anim/animset.h"
#include "graphics/face.h"
#include "math/mathops.h"
#include "engine/msgevent.h"
#include "planvm/planfile.h"
#include "planvm/plannode.h"
#include "common/stub.h"
#include "graphics/modelcomponent.h"
#include "engine/objectphase.h"
#include "engine/modelids.h"
#include <SDL3/SDL.h>

float g_flUnusedEngineObjectA = g_flZeroEngineObject;
float g_flUnusedEngineObjectB = g_flZeroEngineObject;
float g_flUnusedEngineObjectC = g_flZeroEngineObject;

// One collision in five posts teammate or opponent commentary; the two coin tosses differ, as in the original.
// FUNCTION: HYPERX 0x00406620
void EngineObject::PostCollisionCommentary(EngineObject* pA, EngineObject* pB)
{
    short nTeam;
    bool  bSameSide;

    if (rand() % 5 != 0)
    {
        return;
    }

    if ((pA->GetSlot() & 1) != 0)
    {
        bSameSide = (pB->GetSlot() & 1) != 0;
    }
    else
    {
        bSameSide = (pB->GetSlot() & 1) == 0;
    }

    if (bSameSide)
    {
        const int nGenderA = (int)((PlayerObject*)pA)->m_stats.GetScalar(RATING_GENDER);
        int       nGenderB = (int)((PlayerObject*)pB)->m_stats.GetScalar(RATING_GENDER);

        if ((nGenderA & nGenderB) != 0)
        {
            return;
        }

        if ((rand() & 1) != 0)
        {
            pB = pA;
        }

        if ((int)((PlayerObject*)pB)->m_stats.GetScalar(RATING_GENDER) >= 1)
        {
            return;
        }

        if ((pA->GetSlot() & 1) != 0)
        {
            nTeam = (short)g_pGameEngine->m_nTeamAwayIndex;
        }
        else
        {
            nTeam = (short)g_pGameEngine->m_nTeamHomeIndex;
        }

        g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN]->PostSceneEvent(MSG_TEAMMATE_COLLISION, nTeam);

        return;
    }

    if (rand() % 2 != 0)
    {
        pB = pA;
    }

    if ((pB->GetSlot() & 1) != 0)
    {
        nTeam = (short)g_pGameEngine->m_nTeamAwayIndex;
    }
    else
    {
        nTeam = (short)g_pGameEngine->m_nTeamHomeIndex;
    }

    if ((int)((PlayerObject*)pB)->m_stats.GetScalar(RATING_GENDER) >= 1)
    {
        nTeam += g_pGameEngine->m_pTeamTable->GetTeamCount();
    }

    g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN]->PostSceneEvent(MSG_OPPONENT_COLLISION, nTeam);
}

// Two of the three band boundaries, as doubles; the third and the side test's zero are literals.
#define COS_135_DEGREES -0.707
#define COS_60_DEGREES  0.5

// Answers n on one side and 7 - n on the other; the side test uses only X and Y.
// FUNCTION: HYPERX 0x00406a90
int EngineObject::GetApproachAngleSector(EngineObject* pSelf, EngineObject* pOther)
{
    Vec3 vDir;
    int  nSector;

    vDir.x = pOther->GetPositionPtr()->x - pSelf->GetPositionPtr()->x;
    vDir.y = pOther->GetPositionPtr()->y - pSelf->GetPositionPtr()->y;
    vDir.z = pOther->GetPositionPtr()->z - pSelf->GetPositionPtr()->z;

    Math::Vec3Normalize(&vDir);

    Vec3*       pVelDir = &pSelf->GetPhysicsBody()->velDir;
    const float flDot   = Vec3::DotInline(&vDir, pVelDir);

    if (flDot < COS_135_DEGREES)
    {
        nSector = 0;
    }
    else if (flDot < 0.0)
    {
        nSector = 1;
    }
    else
    {
        nSector = 2;

        if (flDot >= COS_60_DEGREES)
        {
            nSector = 3;
        }
    }

    pVelDir = &pSelf->GetPhysicsBody()->velDir;

    if (vDir.y * pVelDir->x - vDir.x * pVelDir->y > 0.0)
    {
        return nSector;
    }

    return 7 - nSector;
}

inline void EngineObject::StoreVector(Vec3* pDest, float flX, float flY, float flZ)
{
    pDest->x = flX;
    pDest->y = flY;
    pDest->z = flZ;
}

// FUNCTION: HYPERX 0x00422df0
void EngineObject::SetEntitySlot(int nEntitySlot)
{
    m_nEntitySlot = nEntitySlot;
}

// FUNCTION: HYPERX 0x00422e00
AgendaItem* EngineObject::GetObserver()
{
    return m_pBody->pTarget;
}

// FUNCTION: HYPERX 0x00422e10
void EngineObject::SetObserver(AgendaItem* pObserver)
{
    m_pBody->pTarget = pObserver;
}

// FUNCTION: HYPERX 0x00422e20
short EngineObject::GetShadowCount()
{
    return m_wShadowCount;
}

// FUNCTION: HYPERX 0x00422e30
CPlayer* EngineObject::GetPhysStateObj()
{
    return m_pBody->pPlayer;
}

// FUNCTION: HYPERX 0x00422e40
std::recursive_mutex* EngineObject::GetCPlayerStateLock()
{
    return &m_pBody->pPlayer->m_lock;
}

// FUNCTION: HYPERX 0x00422e60
Vec3* EngineObject::GetPositionPtr()
{
    return &m_pBody->pos;
}

// FUNCTION: HYPERX 0x00422e70
Vec3* EngineObject::GetWorldPos(Vec3* pOut)
{
    Vec3 vPos;

    const TransformObject* pWorld = &m_pBones->m_world;

    StoreVector(&vPos, pWorld->m_a[0][3], pWorld->m_a[1][3], pWorld->m_a[2][3]);

    *pOut = vPos;

    return pOut;
}

// FUNCTION: HYPERX 0x00422ec0
PhysicsBody* EngineObject::GetPhysicsBody()
{
    return m_pBody;
}

// FUNCTION: HYPERX 0x00422ed0
TransformObject* EngineObject::GetLocalTransformPtr()
{
    return &m_transformB;
}

// FUNCTION: HYPERX 0x00422ee0
ObjectPhase EngineObject::GetPhase()
{
    return m_nPhase;
}

// FUNCTION: HYPERX 0x00422ef0
void EngineObject::StartClipPlayback(AnimDB* pClip, float flRate, int nFlags)
{
    m_pAnimPlayer->StartClipPlayback(pClip, flRate, nFlags);
}

// FUNCTION: HYPERX 0x00422f10
void EngineObject::BindAnimEventChannel(int nChannel, int nTarget, int nKind, int nParam)
{
    m_pAnimPlayer->AssignAnimEventSlot(nChannel, nTarget, nKind, nParam);
}

// FUNCTION: HYPERX 0x00422f30
short EngineObject::GetSlot()
{
    return (short)m_nSlot;
}

// FUNCTION: HYPERX 0x00422f40
PlanState* EngineObject::GetPlanState()
{
    return m_pPlanState;
}

// The event names no target object.
// FUNCTION: HYPERX 0x00422f50
void EngineObject::PostSceneEvent(int nMessage, int nArg)
{
    PostSceneMessage(nMessage, nArg, MODEL_ID_NONE, 0);
}

// FUNCTION: HYPERX 0x00422f70
char EngineObject::IsVisible()
{
    return m_bVisible;
}

// FUNCTION: HYPERX 0x00422f80
void EngineObject::SetVisible(char bFlag)
{
    m_bVisible = bFlag;
}

// FUNCTION: HYPERX 0x00425520
void EngineObject::PostEvent(int nMessage, int nArg, int nSender)
{
    PostSceneMessage(nMessage, nArg, nSender, 0);
}

// The original's linker folds CCircleTrack::Release and InfoBar::SetPassBarMarker onto this.
// FUNCTION: HYPERX 0x0040b900
void EngineObject::CheckObjectAI(EngineObject*)
{
}

// FUNCTION: HYPERX 0x0046bd10
short EngineObject::GetComponentCount()
{
    return m_wBoneCount;
}

// FUNCTION: HYPERX 0x0046fe60
EngineObject::EngineObject(int nSlot, class ObjectDef* pDef)
{
    m_nUnusedC        = 0;
    m_nUnusedB        = 0;
    m_nUnusedA        = 0;
    m_nLastGoalTickMs = 0;

    InitFields();

    m_pAnimPlayer->SetObject(this);

    m_pBody = new PhysicsBody;

    m_pBody->BindToObject(this, pDef);

    InitModel(nSlot, pDef);

    const char* pszName = pDef->m_pszName;

    m_pszName = strcpy(new char[strlen(pszName) + 1], pszName);

    if (pDef->m_pCollisionList != nullptr)
    {
        InitCollisionBubble(pDef->m_pCollisionList);
    }

    m_nUnusedPhase = OBJECT_PHASE_LOCAL;
    m_nPhase       = OBJECT_PHASE_LOCAL;
}

#define RADIUS_PADDING 1.2f

// Dereferences a null mesh's vertex count, as the original does; the radius is padded by a fifth.
// FUNCTION: HYPERX 0x0046ffc0
void EngineObject::ComputeBoundingSphere()
{
    MeshBone*        pBone;
    const MeshBlock* pMesh;
    const Vec3*      pVert;
    const Vec3*      pCur;
    int              iBone;
    short            iVert;
    Vec3             vSum;
    Vec3             vP;

    Vec3* const pSaved = &m_pBody->pos;
    Vec3        vSaved = *pSaved;

    m_pBody->pos.x = m_pBody->pos.y = m_pBody->pos.z = g_flZeroEngineObject;

    m_bVisible = 1;

    ApplyRenderSlotPose();

    m_pBody->pos = vSaved;

    int nTotal = 0;

    vSum.z = g_flZeroEngineObject;
    vSum.y = g_flZeroEngineObject;
    vSum.x = g_flZeroEngineObject;

    for (iBone = 0; iBone < GetComponentCount(); iBone++)
    {
        pBone = m_pBones + (short)iBone;

        if (pBone->IsUnbound() == 0)
        {
            pMesh = pBone->m_pMesh;

            if (pMesh == nullptr || pMesh->vertexOffset.nBytes == 0)
            {
                pVert = nullptr;
            }
            else
            {
                pVert = pMesh->Vertices();
            }

            const short nVertexCount = pMesh->nVertexCount;

            for (iVert = 0; iVert < nVertexCount; iVert++)
            {
                nTotal++;

                pCur = &pVert[iVert];

                Vec3 vV = *pCur;

                vP.x = vV.x * pBone->m_world.m_a[0][0] + vV.y * pBone->m_world.m_a[0][1] + vV.z * pBone->m_world.m_a[0][2] + pBone->m_world.GetElement(0, 3);
                vP.y = vV.x * pBone->m_world.m_a[1][0] + vV.y * pBone->m_world.m_a[1][1] + vV.z * pBone->m_world.m_a[1][2] + pBone->m_world.m_a[1][3];
                vP.z = vV.x * pBone->m_world.m_a[2][0] + vV.y * pBone->m_world.m_a[2][1] + vV.z * pBone->m_world.m_a[2][2] + pBone->m_world.m_a[2][3];

                vSum.x += vP.x;
                vSum.y += vP.y;
                vSum.z += vP.z;
            }
        }
    }

    const auto flCount = (float)(short)nTotal;

    m_flBoundRadius = 0.0f;
    m_vCentre.x     = vSum.x / flCount;
    m_vCentre.y     = vSum.y / flCount;
    m_vCentre.z     = vSum.z / flCount;

    for (iBone = 0; iBone < GetComponentCount(); iBone++)
    {
        pBone = m_pBones + (short)iBone;

        if (pBone->IsUnbound() == 0)
        {
            pMesh = pBone->m_pMesh;

            if (pMesh == nullptr || pMesh->vertexOffset.nBytes == 0)
            {
                pVert = nullptr;
            }
            else
            {
                pVert = pMesh->Vertices();
            }

            for (iVert = 0; iVert < pBone->m_pMesh->nVertexCount; iVert++)
            {
                pCur = &pVert[iVert];

                Vec3 vQ = *pCur;

                vP.x = vQ.x * pBone->m_world.m_a[0][0] + vQ.y * pBone->m_world.m_a[0][1] + vQ.z * pBone->m_world.m_a[0][2] + pBone->m_world.m_a[0][3];
                vP.y = vQ.x * pBone->m_world.m_a[1][0] + (vQ.y * pBone->m_world.m_a[1][1] + vQ.z * pBone->m_world.m_a[1][2]) + pBone->m_world.m_a[1][3];
                vP.z = vQ.x * pBone->m_world.m_a[2][0] + vQ.y * pBone->m_world.m_a[2][1] + vQ.z * pBone->m_world.m_a[2][2] + pBone->m_world.m_a[2][3];

                const float flDx = vP.x - m_vCentre.x;
                const float flDy = vP.y - m_vCentre.y;
                const float flDz = vP.z - m_vCentre.z;

                const auto flDist = (float)sqrt(static_cast<double>(flDx * flDx + flDy * flDy + flDz * flDz));

                if (flDist > m_flBoundRadius)
                {
                    m_flBoundRadius = flDist;
                }
            }
        }
    }

    m_flBoundRadius *= RADIUS_PADDING;
}

#define OBJ_TABLE_EMPTY (-1)

// The binding is the only allocation; the virtual call reaches the class being built.
// FUNCTION: HYPERX 0x004702f0
void EngineObject::InitFields()
{
    int nEntry;

    m_nSlot      = 0;
    m_nKind      = 0;
    m_pMeshBlob  = nullptr;
    m_wBoneCount = 0;

    m_pAnimPlayer = new AnimPlayer;

    m_wLodLevel        = 0;
    m_cUnused          = 0;
    m_flLodDistSq      = 0.0f;
    m_pShadowPoints    = nullptr;
    m_ppShadowXforms   = nullptr;
    m_wShadowCount     = 0;
    m_pShadowFaceList  = nullptr;
    m_wShadowFaceCount = 0;
    m_pModelBlock      = nullptr;

    m_vCentre.x = m_vCentre.y = m_vCentre.z = g_flZeroEngineObject;
    m_flBoundRadius                         = g_flZeroEngineObject;

    m_wUnused = 0;

    m_vUnusedA.x = m_vUnusedA.y = m_vUnusedA.z = g_flZeroEngineObject;

    m_wRenderSlot = 0;

    m_vUnusedB.x = m_vUnusedB.y = m_vUnusedB.z = g_flZeroEngineObject;

    m_pBones        = nullptr;
    m_bScaledBounds = 0;

    m_vUnusedC.x = m_vUnusedC.y = m_vUnusedC.z = g_flZeroEngineObject;

    m_pBubble        = nullptr;
    m_nPhase         = OBJECT_PHASE_BUILDING;
    m_nUnusedPhase   = OBJECT_PHASE_BUILDING;
    m_bVisible       = 0;
    m_nFlags         = 0;
    m_pBody          = nullptr;
    m_pPlanState     = nullptr;
    m_papszPlanNames = nullptr;
    m_pPlanStates    = nullptr;
    m_ppPlanBlobs    = nullptr;
    m_nPlanCount     = 0;

    m_wVisibleFaceTotal = 0;

    m_pszName = nullptr;

    m_nTypeId          = MODEL_ID_NONE;
    m_nEntitySlot      = 0;
    m_wSyncedPlan      = 0;
    m_wSyncedBehaviour = 0;

    ResetSceneMessageHandlers();

    m_nSceneCount = 0;
    m_nSceneHead  = 0;

    for (nEntry = 0; nEntry < OBJECT_RING_LENGTH; nEntry++)
    {
        m_anSceneMessage[nEntry] = OBJ_TABLE_EMPTY;
        m_anSceneArg[nEntry]     = OBJ_TABLE_EMPTY;
    }

    m_nCommandCount = 0;
    m_nCommandHead  = 0;

    for (nEntry = 0; nEntry < OBJECT_RING_LENGTH; nEntry++)
    {
        m_anCommand[nEntry]    = OBJ_TABLE_EMPTY;
        m_anCommandArg[nEntry] = OBJ_TABLE_EMPTY;
    }

    m_nLastSendMs      = 0;
    m_sendCursor.nWord = 0;
}

// The name is copied before the release, which frees the record it lives in.
// FUNCTION: HYPERX 0x004704e0
EngineObject::~EngineObject()
{
    HwModels::Forget(*this);

    if (HasNoModel() == 0)
    {
        const char* const pName = m_pLod->m_pRecord->m_szName;

        char* const pCopy = strcpy(new char[strlen(pName) + 1], pName);

        if (g_pTexManager->ReleaseLOD(m_pLod, m_pModelBlock) == 0)
        {
            // STRING: HYPERX 0x004c4fd4
            g_gameApp.Fatal("Error deactivating model %s. Check model directory.", pCopy);

            delete[] pCopy;
        }
        else
        {
            delete[] pCopy;
        }
    }

    if (m_pszName != nullptr)
    {
        delete[] m_pszName;

        m_pszName = nullptr;
    }

    if (m_pBones != nullptr)
    {
        delete[] m_pBones;

        m_pBones = nullptr;
    }

    if (m_pBubble != nullptr)
    {
        if (m_pBubble->m_pCage != nullptr)
        {
            ::operator delete(m_pBubble->m_pCage);

            m_pBubble->m_pCage = nullptr;
        }

        delete m_pBubble;

        m_pBubble = nullptr;
    }

    if (m_ppShadowXforms != nullptr)
    {
        delete[] m_ppShadowXforms;

        m_ppShadowXforms = nullptr;
    }

    if (m_ppPlanBlobs != nullptr)
    {
        short nCount = 0;

        while (m_ppPlanBlobs[nCount] != nullptr)
        {
            nCount++;
        }

        for (short nIndex = nCount - 1; nIndex >= 0; nIndex--)
        {
            if (m_ppPlanBlobs[nIndex] != nullptr)
            {
                delete[] m_ppPlanBlobs[nIndex];
            }
        }

        delete[] m_ppPlanBlobs;
    }

    if (m_pPlanStates != nullptr)
    {
        delete[] m_pPlanStates;

        m_pPlanStates = nullptr;
    }

    if (m_papszPlanNames != nullptr)
    {
        while (m_nPlanCount-- != 0)
        {
            delete[] m_papszPlanNames[m_nPlanCount];
        }

        delete[] m_papszPlanNames;
    }

    if (m_pAnimPlayer != nullptr)
    {
        delete m_pAnimPlayer;

        m_pAnimPlayer = nullptr;
    }

    if (m_pBody != nullptr)
    {
        delete m_pBody;

        m_pBody = nullptr;
    }
}

// The slot's low bit is its team, so the palette follows team colours.

#define MATERIAL_SHARED 2

// The only writer of m_ppPlanBlobs, m_papszPlanNames and m_nPlanCount; a model named "null" is a success.
// FUNCTION: HYPERX 0x00470790
char EngineObject::InitModel(int nSlot, ObjectDef* pDef)
{
    char          bRemap;
    unsigned int  nColour;
    unsigned char nIndex;
    int           nShifted;

    m_nSlot         = nSlot;
    m_bScaledBounds = 0;
    m_nKind         = pDef->m_nTypeId;

    if (pDef->m_nPlanCount > 0)
    {
        void* pUnused;

        m_ppPlanBlobs = new PlanWord*[pDef->m_nPlanCount + 1];

        for (short i = 0; i < pDef->m_nPlanCount; i++)
        {
            m_ppPlanBlobs[i] = PlanFile::OpenBinary(pDef->m_apszPlanNames[i], &pUnused);
        }

        m_ppPlanBlobs[pDef->m_nPlanCount] = nullptr;
        m_papszPlanNames                  = pDef->m_apszPlanNames;
        m_nPlanCount                      = pDef->m_nPlanCount;
        pDef->m_apszPlanNames             = nullptr;
        pDef->m_nPlanCount                = 0;
    }

    // STRING: HYPERX 0x004c5148
    if (SDL_strcasecmp(pDef->GetModel(), "null") == 0)
    {
        m_nFlags |= ENGINEOBJECT_NO_MODEL | ENGINEOBJECT_HIDDEN;

        return 1;
    }

    LODNode* const pNode = g_pTexManager->LoadLOD(pDef->GetModel());

    if (pNode != nullptr)
    {
        (pNode->m_wRefCount)++;
    }

    m_pLod = pNode;

    if (pNode == nullptr)
    {
        // STRING: HYPERX 0x004c5104
        g_gameApp.Fatal("Could not find model: %s in model database.  Check model directory.",
                        pDef->GetModel());

        return 0;
    }

    m_pModelBlock = g_pTexManager->FindOrLoadMaterialList(pNode, pDef->m_pszMaterial);

    if (m_pModelBlock == nullptr)
    {
        char* const pszMaterial = pDef->m_pszMaterial;

        if (pszMaterial == nullptr)
        {
            // STRING: HYPERX 0x004c50d8
            g_gameApp.Fatal("Could not get material list for model: %s!",
                            m_pLod->m_pRecord->m_szName);

            return 0;
        }

        // STRING: HYPERX 0x004c50a8
        g_gameApp.Fatal("Could not get material list %s for model: %s!", pszMaterial, m_pLod->m_pRecord->m_szName);

        return 0;
    }

    if (pDef->m_bOwnPalette != 0)
    {
        bRemap = (char)m_nSlot & 1;

        if (pDef->m_bForceRemap != 0)
        {
            bRemap = 1;
        }

        g_pColorPalette->LoadPaletteEntries(pDef->m_pszModel, bRemap);
    }

    // The remap runs once per list; a shared list was remapped by whichever object loaded it first.
    if (m_pModelBlock->m_nRefs < MATERIAL_SHARED)
    {
        bRemap = (char)m_nSlot & 1;

        if (pDef->m_bForceRemap != 0)
        {
            bRemap = 1;
        }

        for (int j = 0; j < m_pModelBlock->m_nMatCount; j++)
        {
            if (g_pColorPalette->IsEntryUsable(m_pModelBlock->m_pMaterials[j].nColour & 0xff) == 0)
            {
                // STRING: HYPERX 0x004c503c
                g_gameApp.Error("ERROR! Material index %d does not reference a valid starting ramp value!\n\nModel File\t\t%s\nMaterial File\t\t%s\n",
                                m_pModelBlock->m_pMaterials[j].nColour &
                                    0xff,
                                pDef->m_pszModel,
                                pDef->m_pszMaterial);
            }

            nColour = m_pModelBlock->m_pMaterials[j].nColour & 0xff;

            if (bRemap != 0)
            {
                if ((int)nColour >= g_pColorPalette->m_remapRange.nFirst &&
                    (int)nColour <= g_pColorPalette->m_remapRange.nCount)
                {
                    nShifted = g_pColorPalette->m_teamARange.nCount + (int)nColour;
                }
                else
                {
                    nShifted = (int)nColour;
                }

                nIndex = (unsigned char)nShifted;
            }
            else
            {
                nIndex = (unsigned char)nColour;
            }

            nColour = nIndex;

            m_pModelBlock->m_pMaterials[j].nColour = nColour;
        }
    }

    ModelComponent* const pBlob = m_pLod->m_pRecord;

    m_pMeshBlob = pBlob;

    ApplyHeaderRotation(pBlob);

    const short nBones = m_pMeshBlob->m_nSubComponents;

    m_wBoneCount = nBones;

    auto* const pBones = new MeshBone[nBones];

    m_pBones = pBones;

    if (InitRenderSlotsAndShadowLinks(m_pLod, pBones, g_pTexManager, 0) != 0)
    {
        InitShadowVertexLinks(m_pLod, m_pMeshBlob);

        CountVisibleFaces();

        m_bVisible = 1;
        m_nFlags   = 0;

        ComputeBoundingSphere();

        HwModels::Load(*this, *pDef);

        return 1;
    }

    // STRING: HYPERX 0x004c5008
    g_gameApp.Fatal("InitModel: Could not find component for Model %s!",
                    m_pLod->m_pRecord->m_szName);

    return 0;
}

inline void EngineObject::FreeName(char*& pszName)
{
    if (pszName != nullptr)
    {
        delete[] pszName;

        pszName = nullptr;
    }
}

// InitModel's second half; the bone array is rebuilt only when the bone count changes.
// FUNCTION: HYPERX 0x00470b40
char EngineObject::ReInitModel(int nSlot, ObjectDef* pDef)
{
    char          bRemap;
    unsigned int  nColour;
    unsigned char nIndex;
    int           nShifted;

    m_nSlot         = nSlot;
    m_bScaledBounds = 0;

    m_transformA.ResetToIdentity();

    // The release frees the record this name lives in, so the copy has to come first.
    const char* const pszLodName = m_pLod->m_pRecord->m_szName;

    char* const pszOldName = strcpy(new char[strlen(pszLodName) + 1], pszLodName);

    if (g_pTexManager->ReleaseLOD(m_pLod, m_pModelBlock) == 0)
    {
        // STRING: HYPERX 0x004c4fd4
        g_gameApp.Fatal("Error deactivating model %s. Check model directory.", pszOldName);

        delete[] pszOldName;

        return 0;
    }

    delete[] pszOldName;

    // STRING: HYPERX 0x004c5148
    if (SDL_strcasecmp(pDef->GetModel(), "null") == 0)
    {
        m_nFlags |= ENGINEOBJECT_NO_MODEL | ENGINEOBJECT_HIDDEN;

        return 1;
    }

    LODNode* const pNode = g_pTexManager->LoadLOD(pDef->GetModel());

    if (pNode != nullptr)
    {
        (pNode->m_wRefCount)++;
    }

    m_pLod = pNode;

    if (pNode == nullptr)
    {
        // STRING: HYPERX 0x004c5104
        g_gameApp.Fatal("Could not find model: %s in model database.  Check model directory.",
                        pDef->GetModel());

        return 0;
    }

    m_pModelBlock = g_pTexManager->FindOrLoadMaterialList(pNode, pDef->m_pszMaterial);

    if (m_pModelBlock == nullptr)
    {
        char* const pszMaterial = pDef->m_pszMaterial;

        if (pszMaterial == nullptr)
        {
            // STRING: HYPERX 0x004c50d8
            g_gameApp.Fatal("Could not get material list for model: %s!",
                            m_pLod->m_pRecord->m_szName);

            return 0;
        }

        // STRING: HYPERX 0x004c50a8
        g_gameApp.Fatal("Could not get material list %s for model: %s!", pszMaterial, m_pLod->m_pRecord->m_szName);

        return 0;
    }

    if (m_pszName != nullptr)
    {
        EngineObject::FreeName(m_pszName);
    }

    // The client holds the name in ESI across operator new (0x00470cce to 0x00470ce3).
    const char* const pszName = pDef->m_pszName;

    m_pszName = strcpy(new char[strlen(pszName) + 1], pszName);

    if (pDef->m_bOwnPalette != 0)
    {
        bRemap = (char)m_nSlot & 1;

        if (pDef->m_bForceRemap != 0)
        {
            bRemap = 1;
        }

        g_pColorPalette->LoadPaletteEntries(pDef->m_pszModel, bRemap);
    }

    // The remap runs once per list; a shared list was remapped by whichever object loaded it first.
    if (m_pModelBlock->m_nRefs < MATERIAL_SHARED)
    {
        bRemap = (char)m_nSlot & 1;

        if (pDef->m_bForceRemap != 0)
        {
            bRemap = 1;
        }

        for (int j = 0; j < m_pModelBlock->m_nMatCount; j++)
        {
            if (g_pColorPalette->IsEntryUsable(m_pModelBlock->m_pMaterials[j].nColour & 0xff) == 0)
            {
                // STRING: HYPERX 0x004c503c
                g_gameApp.Error("ERROR! Material index %d does not reference a valid starting ramp value!\n\nModel File\t\t%s\nMaterial File\t\t%s\n",
                                m_pModelBlock->m_pMaterials[j].nColour &
                                    0xff,
                                pDef->m_pszModel,
                                pDef->m_pszMaterial);
            }

            nColour = m_pModelBlock->m_pMaterials[j].nColour & 0xff;

            if (bRemap != 0)
            {
                if ((int)nColour >= g_pColorPalette->m_remapRange.nFirst &&
                    (int)nColour <= g_pColorPalette->m_remapRange.nCount)
                {
                    nShifted = g_pColorPalette->m_teamARange.nCount + (int)nColour;
                }
                else
                {
                    nShifted = (int)nColour;
                }

                nIndex = (unsigned char)nShifted;
            }
            else
            {
                nIndex = (unsigned char)nColour;
            }

            nColour = nIndex;

            m_pModelBlock->m_pMaterials[j].nColour = nColour;
        }
    }

    ModelComponent* const pBlob = m_pLod->m_pRecord;

    m_pMeshBlob = pBlob;

    ApplyHeaderRotation(pBlob);

    m_nKind = pDef->m_nTypeId;

    const short nOldBones = m_wBoneCount;

    m_wBoneCount = m_pMeshBlob->m_nSubComponents;

    if (nOldBones != m_wBoneCount)
    {
        m_transformC.ResetToIdentity();
        m_transformB.ResetToIdentity();

        delete[] m_pBones;

        m_pBones = new MeshBone[m_wBoneCount];
    }

    if (InitRenderSlotsAndShadowLinks(m_pLod, m_pBones, g_pTexManager, nOldBones == m_wBoneCount) != 0)
    {
        InitShadowVertexLinks(m_pLod, m_pMeshBlob);

        CountVisibleFaces();
        ComputeBoundingSphere();

        HwModels::Load(*this, *pDef);

        return 1;
    }

    // STRING: HYPERX 0x004c5150
    g_gameApp.Fatal("ReInitModel: Could not find component for Model %s!",
                    m_pLod->m_pRecord->m_szName);

    return 0;
}

// A plan that will not compile stops the whole table there.
// FUNCTION: HYPERX 0x00470fb0
void EngineObject::CompileAllPlans()
{
    PlanWord** ppSource = m_ppPlanBlobs;

    if (ppSource != nullptr)
    {
        m_pPlanStates = new PlanState[m_nPlanCount];

        ppSource = m_ppPlanBlobs;

        for (int nIndex = 0; nIndex < m_nPlanCount; nIndex++)
        {
            m_pPlanStates[nIndex].m_pOwner = this;

            m_pPlanStates[nIndex].m_pszName = m_papszPlanNames[nIndex];

            m_pPlanStates[nIndex].m_wSyncValue = (short)nIndex;

            PlanWord* const pSource = *ppSource++;

            PlanNode** const ppCompiled = m_pPlanStates[nIndex].Compile(pSource);

            if (ppCompiled == nullptr)
            {
                g_gameApp.Error(
                    // STRING: HYPERX 0x004c5184
                    "Failed to compile %s. in %s\n",
                    m_papszPlanNames[nIndex],
                    m_pszName);

                return;
            }

            m_pPlanStates[nIndex].SetOwnerAndReset(this, ppCompiled);
        }
    }
}

// Answers minus one when not found, which nothing stops a caller indexing with.
// FUNCTION: HYPERX 0x004710f0
short EngineObject::GetBehaviorIndex(PlanState* pState, const char* pszName)
{
    short iNode = 0;

    GameBehaviorObject* pNode = pState->m_pHead;

    while (pNode != nullptr)
    {
        if (SDL_strcasecmp(pNode->GetName(), pszName) == 0)
        {
            break;
        }

        pNode = pNode->m_pStandby;

        iNode++;
    }

    if (pNode != nullptr)
    {
        return iNode;
    }

    // STRING: HYPERX 0x004c51a4
    g_gameApp.Fatal("Could not find Behavior named %s!", pszName);

    return -1;
}

// A plan named in the wrong case is not found; the miss answers minus one.

// FUNCTION: HYPERX 0x00471150
short EngineObject::FindPlanByName(const char* pszName)
{
    short iPlan;

    for (iPlan = 0; iPlan < m_nPlanCount; iPlan++)
    {
        if (strcmp(pszName, GetPlanName(iPlan)) == 0)
        {
            break;
        }
    }

    if (iPlan < m_nPlanCount)
    {
        return iPlan;
    }

    // STRING: HYPERX 0x004c51c8
    g_gameApp.Error("Could not find plan named %s!", pszName);

    return -1;
}

#define COMMAND_NONE 0xffffffffu

#define RECORD_MARK 0xff

// The lock is released before each record is used, because the work can call back into the queue.
// FUNCTION: HYPERX 0x004711c0
char EngineObject::ProcessReceivedPacket()
{
    NetMoveRecord record = {};
    NetMoveBody   body;
    CPlayer*      pPlayer;
    BoundBubble*  pColObj;
    AttachPak*    pAttach;

    char bResult = 0;

    short nPlan      = m_pPlanState->m_wSyncValue;
    short nBehaviour = m_pPlanState->m_pCurrent->m_wSyncValue;

    pPlayer = m_pBody->pPlayer;

    if (pPlayer != nullptr)
    {
        pPlayer->m_lock.lock();

        NetMoveRecord* pRecord = pPlayer->DequeueNetRecord();

        if (pRecord != nullptr)
        {
            record = *pRecord;
        }

        m_pBody->pPlayer->m_lock.unlock();

        if (pRecord != nullptr)
        {
            do
            {
                body = record.body;

                if (record.body.nPlan == RECORD_MARK && record.body.nBehaviour == RECORD_MARK)
                {
                    m_pBody->step = record.body.vVelocity;

                    m_pBody->netTarget = body.vPosition;
                }

                m_pBody->bNetTargetPending = 1;

                m_pBody->nPrevTick = record.nTime;

                ApplySyncedBehaviour(pPlayer, &body, &nPlan, &nBehaviour);

                if (body.nCommand != COMMAND_NONE)
                {
                    if (pPlayer->pState->nObjectSlot != SLOT_GOD)
                    {
                        EnqueueInputCommand(body.nCommand, body.nPayload);
                    }
                    else
                    {
                        g_pGameEngine->m_ppSlots[NetSequenceWord::GetTarget(body.nSequence)]
                            ->EnqueueInputCommand(body.nCommand, body.nPayload);
                    }

                    if (body.nSecondCommand != COMMAND_NONE)
                    {
                        if (pPlayer->pState->nObjectSlot != SLOT_GOD)
                        {
                            EnqueueInputCommand(body.nSecondCommand, body.nSecondPayload);
                        }
                        else
                        {
                            g_pGameEngine->m_ppSlots[NetSequenceWord::GetSecondTarget(body.nSequence)]
                                ->EnqueueInputCommand(body.nSecondCommand, body.nSecondPayload);
                        }
                    }
                }

                if (pPlayer->pState->nObjectSlot == SLOT_ROK && m_nPhase == OBJECT_PHASE_NETWORK)
                {
                    const unsigned int nCamera = NetSequenceWord::GetCarrier(body.nSequence);

                    if ((short)nCamera != 0)
                    {
                        pColObj = g_pGameEngine->GetColObjPtr(
                            (short)AnimSet::GetFieldOffset((short)(nCamera - 1)));

                        pAttach = pColObj->m_pAttach;

                        g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach = pAttach;
                    }
                    else
                    {
                        g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach = nullptr;
                    }
                }

                pPlayer->m_lock.lock();

                pRecord = pPlayer->DequeueNetRecord();

                if (pRecord != nullptr)
                {
                    record = *pRecord;
                }

                m_pBody->pPlayer->m_lock.unlock();
            } while (pRecord != nullptr);

            bResult = 1;
        }
    }

    return bResult;
}

#define STATE_NONE 0xff

// The referee applies a signed nibble to every stationary object; 0xff is reserved.
// FUNCTION: HYPERX 0x00471460
void EngineObject::ApplySyncedBehaviour(CPlayer* pOwner, const NetMoveBody* pBody, short* pOutPlan, short* pOutBehaviour)
{
    const int nPlan = pBody->nPlan;

    if (nPlan == STATE_NONE)
    {
        return;
    }

    if (m_nPhase != OBJECT_PHASE_NETWORK)
    {
        return;
    }

    if (pOwner->pState->nObjectSlot != SLOT_GOD)
    {
        if (*pOutPlan != (short)pBody->nPlan || *pOutBehaviour != (short)pBody->nBehaviour)
        {
            EnqueueInputCommand(COMMAND_SET_PLAN_AND_BEHAVIOR, PlanBehaviourWord::Pack(nPlan, pBody->nBehaviour));

            *pOutPlan      = (short)pBody->nPlan;
            *pOutBehaviour = (short)pBody->nBehaviour;
        }

        return;
    }

    for (int nIndex = 0; nIndex < g_nStationaryCount; nIndex++)
    {
        EngineObject* const pObject = g_pGameEngine->m_ppSlots[(short)g_aStationarySlots[nIndex]];

        const short nCurrent = pObject->GetPlanState()->m_pCurrent->m_wSyncValue;

        const auto nValue = (short)SlotNibbles::Get(pBody->abNibbles, nIndex);

        *pOutBehaviour = nValue;

        if (nCurrent != nValue)
        {
            pObject->EnqueueInputCommand(COMMAND_SET_BEHAVIOR, nValue);
        }
    }
}

// A child id nothing answers to is fatal, as is a zero slot count with children to place.
// FUNCTION: HYPERX 0x00471560
void EngineObject::BuildShadowComponentLinks(MeshBone* pSlot)
{
    short nChild;

    if (pSlot->m_pDesc->m_nChildCount == 0)
    {
        pSlot->m_apChildren = nullptr;

        return;
    }

    pSlot->m_apChildren = new MeshBone*[pSlot->m_pDesc->m_nChildCount];

    MeshBone** ppChild = pSlot->m_apChildren;

    const MeshBoneDesc* pDescriptor = pSlot->m_pDesc;

    for (nChild = 0; nChild < pSlot->m_pDesc->m_nChildCount; nChild++)
    {
        MeshBone* pCandidate = m_pBones;

        short nSlot = 0;

        char bFound = 0;

        while (nSlot < m_wBoneCount && bFound == 0)
        {
            if (pCandidate->m_pDesc->m_nLinkId == pDescriptor->m_anChildIds[nChild])
            {
                *ppChild = pCandidate;
                ppChild++;
                bFound = 1;
            }

            nSlot++;
            pCandidate++;
        }

        if (bFound == 0)
        {
            // STRING: HYPERX 0x004c51e8
            g_gameApp.Fatal("No match for linkID in while setting up model component links!");
        }
    }
}

// One static scratch transform serves every object; 0x004743d0 is the same source in another unit.
// FUNCTION: HYPERX 0x00471630
void EngineObject::UpdateRenderSlotTransform(short nSlotIdx)
{
    // The original's object at 0x004d05c0 takes no marker; symbols/static_guards.csv names its guard.
    static TransformObject SlotTransform;

    m_nFlags |= ENGINEOBJECT_POSED;
    MeshBone* const        pSlot  = &m_pBones[nSlotIdx];
    TransformObject* const pWorld = &m_transformC;

    m_transformB.Multiply(pWorld, &m_transformA);

    pWorld->AddTranslationVec(&m_pBody->pos);

    pSlot->PropagateTransforms(nSlotIdx, pWorld, pWorld, &SlotTransform);
}

// Two threshold tables keep the level from flickering; it moves one step a frame at most.

// FUNCTION: HYPERX 0x004716f0
void EngineObject::UpdateLODAndPushComponents(CameraObject* pCamera)
{
    short nWanted;

    CameraProjectionState* const pViewpoint = pCamera->layout.pCamera;
    MeshBone*                    pComponent = m_pBones;

    const Vec3* pPos = GetPositionPtr();

    const float flDx = pViewpoint->m_vEye.x - pPos->x;
    const float flDy = pViewpoint->m_vEye.y - pPos->y;
    const float flDz = pViewpoint->m_vEye.z - pPos->z;

    LODNode* const pTable  = m_pLod;
    const short    nLevels = pTable->m_wLodCount;

    float flDistSq = (flDz * flDz + flDx * flDx) + flDy * flDy;

    if (nLevels > 1)
    {
        if (flDistSq > m_flLodDistSq && (nWanted = (short)(m_wLodLevel + 1)) < nLevels && flDistSq > pTable->m_aflFarther[nWanted])
        {
            m_wLodLevel   = nWanted;
            m_flLodDistSq = flDistSq;
        }
        else
        {
            if (flDistSq < m_flLodDistSq && m_wLodLevel > 0 && flDistSq < pTable->m_aflNearer[(short)(m_wLodLevel - 1)])
            {
                m_wLodLevel--;
                m_flLodDistSq = flDistSq;
            }
        }
    }

    for (short iComponent = 0; iComponent < m_wBoneCount; iComponent++)
    {
        if (pComponent->IsBound() != 0 && pComponent->IsHidden() == 0)
        {
            pComponent->PushToRenderStack(pCamera);
        }

        pComponent++;
    }
}

// One, read twice here and nowhere else.
// GLOBAL: HYPERX 0x004b38ec
extern const float g_flVertexWDefault;

// The face normal the two renderers read back. Nothing else in the image names it.
// GLOBAL: HYPERX 0x004d05b0
extern Vec3 g_vDispatchFaceNormal;

// Written here and in meshbone.cpp; the mesh and software renderers read it.
extern Vec3 g_vLightDirModel;

extern class TransformObject* g_pCameraTransform;

#define CLIP_OFFSCREEN ((char)-1)
#define CLIP_ANY_PLANE 1

// FUNCTION: HYPERX 0x00471800
void EngineObject::ApplyRenderSlotPose()
{
    UpdateRenderSlotTransform(m_wRenderSlot);
}

// The posed shadow, laid on the arena in world space, goes to the GPU with the camera alone.
void EngineObject::SubmitShadowToHardware() const
{
    // Shadow maps stand in for the shadow meshes.
    if (GraphicsSettings::On(GraphicsSettings::Switch::SHADOW_MAPS))
    {
        return;
    }

    const TransformedVertex* const pPosed = &g_pModelVertexBuf[m_wShadowCount];

    auto points = std::vector<Vec3>((size_t)m_wShadowCount);

    for (int i = 0; i < m_wShadowCount; i++)
    {
        points[i] = Vec3{ pPosed[i].x, pPosed[i].y, pPosed[i].z };
    }

    const auto vertices = HwMesh::BuildShadow(points.data(), m_wShadowCount, m_pShadowFaceList, m_wShadowFaceCount, m_pModelBlock->m_pMaterials, m_pModelBlock->m_nMatCount, g_vDispatchFaceNormal);

    auto draw = HwRenderer::Draw{};

    for (int nRow = 0; nRow < 4; nRow++)
    {
        for (int nColumn = 0; nColumn < 4; nColumn++)
        {
            draw.aPixelFromModel[nRow * 4 + nColumn] = g_pCameraTransform->m_a[nRow][nColumn];
        }
    }

    // Without a fourth row the engine divides by one.
    if (g_pCameraTransform->m_bHasRow3 == 0)
    {
        draw.aPixelFromModel[12] = 0.0f;
        draw.aPixelFromModel[13] = 0.0f;
        draw.aPixelFromModel[14] = 0.0f;
        draw.aPixelFromModel[15] = 1.0f;
    }

    // The points are already in the world, lit by the light the transparent pass picked.
    draw.aWorldFromModel[0]  = 1.0f;
    draw.aWorldFromModel[5]  = 1.0f;
    draw.aWorldFromModel[10] = 1.0f;
    draw.aWorldFromModel[15] = 1.0f;

    draw.aLightDirs[0][0] = g_lightDirs[0].dir.x;
    draw.aLightDirs[0][1] = g_lightDirs[0].dir.y;
    draw.aLightDirs[0][2] = g_lightDirs[0].dir.z;
    draw.aLightDirs[0][3] = 1.0f;

    draw.nViewportX      = g_nViewportOriginX;
    draw.nViewportY      = g_nViewportOriginY;
    draw.nViewportWidth  = g_nPrevWidth;
    draw.nViewportHeight = g_nPrevHeight;
    draw.bPalettised     = true;

    HwRenderer::SubmitVertices(vertices, draw);
}

// Transformed vertices land in the upper half of the model vertex buffer, projected in the lower.

// FUNCTION: HYPERX 0x00471810
void EngineObject::DrawTransformedMesh(CollisionShape* pSurface)
{
    Vec3 vA;
    Vec3 vB;
    Vec3 vC;
    Vec3 vEdge1;
    Vec3 vEdge2;
    Vec3 vNormal;

    if (m_pMeshBlob->m_shadowOffset.nBytes == 0)
    {
        return;
    }

    const Vec3*        pVert = m_pShadowPoints;
    TransformedVertex* pOut  = &g_pModelVertexBuf[m_wShadowCount];

    const TransformObject** ppXf = m_ppShadowXforms;

    g_vLightDirModel = g_lightDirs[0].dir;

    for (short nVert = 0; nVert < m_wShadowCount; nVert++)
    {
        const TransformObject* pXf = *ppXf;
        ppXf++;

        pOut->x = ((pXf->m_a[0][2] * pVert->z) + pXf->m_a[0][1] * pVert->y) + pXf->m_a[0][0] * pVert->x + pXf->m_a[0][3];
        pOut->y = ((pXf->m_a[1][2] * pVert->z) + pXf->m_a[1][1] * pVert->y) + pXf->m_a[1][0] * pVert->x + pXf->m_a[1][3];
        pOut->z = ((pXf->m_a[2][2] * pVert->z) + pXf->m_a[2][1] * pVert->y) + pXf->m_a[2][0] * pVert->x + pXf->m_a[2][3];

        if (pXf->m_bHasRow3 != 0)
        {
            pOut->w = (pXf->m_a[3][2] * pVert->z + pXf->m_a[3][1] * pVert->y) + pXf->m_a[3][0] * pVert->x + pXf->m_a[3][3];
        }
        else
        {
            pOut->w = g_flVertexWDefault;
        }

        pOut->z = pSurface->GetZ(pOut->x, pOut->y);

        pVert++;
        pOut++;
    }

    if (HwRenderer::Capturing())
    {
        SubmitShadowToHardware();

        return;
    }

    const TransformedVertex* pIn = &g_pModelVertexBuf[m_wShadowCount];
    pOut                         = g_pModelVertexBuf;

    for (short nCamera = 0; nCamera < m_wShadowCount; nCamera++)
    {
        const TransformObject* pCam = g_pCameraTransform;

        pOut->x = (pCam->m_a[0][2] * pIn->z + (pCam->m_a[0][1] * pIn->y)) + pCam->m_a[0][0] * pIn->x + pCam->m_a[0][3];
        pOut->y = (pCam->m_a[1][2] * pIn->z + (pCam->m_a[1][1] * pIn->y)) + pCam->m_a[1][0] * pIn->x + pCam->m_a[1][3];
        pOut->z = (pCam->m_a[2][2] * pIn->z + (pCam->m_a[2][1] * pIn->y)) + pCam->m_a[2][0] * pIn->x + pCam->m_a[2][3];

        if (pCam->m_bHasRow3 != 0)
        {
            pOut->w = (pCam->m_a[3][2] * pIn->z + (pCam->m_a[3][1] * pIn->y)) + pCam->m_a[3][0] * pIn->x + pCam->m_a[3][3];
        }
        else
        {
            pOut->w = g_flVertexWDefault;
        }

        pIn++;
        pOut++;
    }

    for (int nFace = 0; nFace < m_wShadowFaceCount; nFace++)
    {
        char cFacing = 1;

        MeshFace* const pFace = &m_pShadowFaceList[(short)nFace].face;

        const char cClip = Face::ClipCodeAndCullCheck(pFace, g_pModelVertexBuf);

        if (cClip == CLIP_OFFSCREEN)
        {
            continue;
        }

        if ((cClip & CLIP_ANY_PLANE) == 0)
        {
            const TransformedVertex* pCorner2 = &g_pModelVertexBuf[pFace->aCorners[2]];
            const TransformedVertex* pCorner1 = &g_pModelVertexBuf[pFace->aCorners[1]];
            const TransformedVertex* pCorner0 = &g_pModelVertexBuf[pFace->aCorners[0]];

            const float flW0 = 1.0f / pCorner0->w;
            vA.x             = flW0 * pCorner0->x;
            vA.y             = flW0 * pCorner0->y;
            vA.z             = flW0 * pCorner0->z;

            const float flW1 = 1.0f / pCorner1->w;
            vB.x             = flW1 * pCorner1->x;
            vB.y             = flW1 * pCorner1->y;
            vB.z             = flW1 * pCorner1->z;

            const float flW2 = 1.0f / pCorner2->w;
            vC.x             = flW2 * pCorner2->x;
            vC.y             = flW2 * pCorner2->y;
            vC.z             = flW2 * pCorner2->z;

            vEdge1.x = vB.x - vA.x;
            vEdge1.y = vB.y - vA.y;
            vEdge1.z = vB.z - vA.z;

            vEdge2.x = vC.x - vA.x;
            vEdge2.y = vC.y - vA.y;
            vEdge2.z = vC.z - vA.z;

            Vec3::CrossProduct(&vEdge2, &vEdge1, &vNormal);

            cFacing = vNormal.z <= 0.0;
        }

        MeshMaterial* apMaterials[2];

        apMaterials[0] = &m_pModelBlock->m_pMaterials[(int)pFace->cMaterial];
        apMaterials[1] = &m_pModelBlock->m_pMaterials[apMaterials[0]->nBackIndex];

        MeshBone::DispatchFace(m_pBones, cFacing, pFace, apMaterials, nullptr, g_pModelVertexBuf, &g_vDispatchFaceNormal, cClip);
    }
}

// A named slot runs Human, an unnamed one Defense; a slot below 0 searches with a null name, unset in the original.
// FUNCTION: HYPERX 0x00471b60
void EngineObject::SelectAndActivatePlan()
{
    const char* pszWanted = nullptr;
    CPlayer*    pPlayer;
    int         nSlot;
    int         nIndex;

    // Cameras reach this with no player, so nothing below runs for them.
    pPlayer = GetPhysicsBody()->pPlayer;

    if (pPlayer != nullptr)
    {
        pPlayer = GetPhysicsBody()->pPlayer;

        if (pPlayer->pState->nConnected != 0)
        {
            m_nPhase = OBJECT_PHASE_NETWORK;
        }

        nSlot = m_nSlot;

        if (nSlot != SLOT_DROMER0 && nSlot != SLOT_DROMER1 && nSlot != SLOT_DROMER2 && nSlot != SLOT_DROMER3)
        {
            InstantiateAIPlan(0);

            m_pPlanState->ActivateBehavior(0);
        }
        else if (pPlayer->pState->nHuman != 0)
        {
            if (pPlayer->pState->nConnected == 0)
            {
                g_pGameEngine->m_apTracked[0] = g_pGameEngine->m_ppSlots[(short)nSlot];
                g_pGameEngine->m_apTracked[1] = g_pGameEngine->m_ppSlots[(short)m_nSlot];
            }

            for (nIndex = 0; nIndex < m_nPlanCount; nIndex++)
            {
                if (SDL_strcasecmp(m_papszPlanNames[nIndex],
                                   // STRING: HYPERX 0x004bb340
                                   "Human") == 0)
                {
                    InstantiateAIPlan((short)nIndex);

                    m_pPlanState->ActivateBehavior(0);

                    break;
                }
            }

            if (nIndex >= m_nPlanCount)
            {
                // STRING: HYPERX 0x004c524c
                g_gameApp.Fatal("Could not find Human plan for %s",
                                m_pszName);
            }
        }
        else
        {
            for (nIndex = 0; nIndex < m_nPlanCount; nIndex++)
            {
                if (SDL_strcasecmp(m_papszPlanNames[nIndex],
                                   // STRING: HYPERX 0x004bb384
                                   "Defense") == 0)
                {
                    InstantiateAIPlan((short)nIndex);

                    m_pPlanState->ActivateBehavior(0);

                    break;
                }
            }

            if (nIndex >= m_nPlanCount)
            {
                // STRING: HYPERX 0x004c5228
                g_gameApp.Fatal("Could not find Defense plan for %s",
                                m_pszName);
            }
        }
    }
    else
    {
        nSlot = m_nSlot;

        if (nSlot < 2)
        {
            if (nSlot == 0)
            {
                pszWanted = "Human";
            }
            else if (nSlot == 1)
            {
                pszWanted = "Defense";
            }

            for (nIndex = 0; nIndex < m_nPlanCount; nIndex++)
            {
                if (SDL_strcasecmp(m_papszPlanNames[nIndex], pszWanted) == 0)
                {
                    InstantiateAIPlan((short)nIndex);

                    break;
                }
            }

            if (nIndex >= m_nPlanCount)
            {
                g_gameApp.Fatal("Could not find Defense plan for %s",
                                m_pszName);
            }
        }
        else
        {
            InstantiateAIPlan(0);
        }
    }

    m_pPlanState->ActivateBehavior(0);

    // The behaviour just activated, which is what m_pCurrent holds.
    m_pPlanState->m_pCurrent->StartMotion();

    if (m_pBubble != nullptr && m_pBubble->m_pCage != nullptr)
    {
        m_pBubble->m_pCage->ComputeFaceNormals();
    }
}

// Retries once with the default minimal plan; the handlers catch AdlibError& and char*.

// FUNCTION: HYPERX 0x00471da0
void EngineObject::InstantiateAIPlan(short nIndex)
{
    PlanState** ppState = &m_pPlanState;

    if (*ppState != nullptr)
    {
        (*ppState)->Teardown();
    }

    *ppState = &m_pPlanStates[nIndex];

    if (*ppState != nullptr)
    {
        try
        {
            (*ppState)->m_ppCursor = (*ppState)->m_ppBehaviors;

            if ((*ppState)->Instantiate() == nullptr)
            {
                // STRING: HYPERX 0x004c528c
                g_gameApp.Error("Plan instantiation failed. Setting %s plan to default minimal.",
                                m_pszName);

                (*ppState)->SetOwnerAndReset(this, nullptr);
                (*ppState)->Reinitialize();
                (*ppState)->Instantiate();
            }
        }
        catch (AdlibError& err)
        {
            switch (err.nCode)
            {
                case 1:
                    g_gameApp.Error(err.pszText);
                    break;

                case 2:
                    g_gameApp.Fatal(err.pszText);
                    break;
            }
        }
        catch (char* pszText)
        {
            g_gameApp.Error(pszText);
        }
    }
}

// Any shape but a sphere, an attached or a caged bubble is fatal.

// FUNCTION: HYPERX 0x00471eb0
void EngineObject::InitCollisionBubble(BubbleCmd* pShape)
{
    BoundBubble* pBubble;
    int          nTag;

    switch (pShape->m_wShape)
    {
        case BUBBLE_SHAPE_SPHERE:
            pBubble   = new BoundBubble(this, pShape);
            m_pBubble = pBubble;
            break;

        case BUBBLE_SHAPE_ATTACHED:
            pBubble   = new BoundBubble(this, pShape);
            m_pBubble = pBubble;
            break;

        case BUBBLE_SHAPE_CAGED:
            pBubble   = new BoundBubble(this, pShape);
            m_pBubble = pBubble;

            // The whole word is tested, since a real AttachPak's low half can carry the tag's bits.
            if (((std::uintptr_t)pBubble->m_pAttach & ~(std::uintptr_t)0xffff) == EXTRA_USE_DETAIL)
            {
                nTag = (int)(std::intptr_t)pBubble->m_pAttach;

                m_pBubble->m_pCage = new CollisionCage(this, nTag & 0xffff);
            }

            break;

        default:
            g_gameApp.Fatal(
                // STRING: HYPERX 0x004c52cc
                "InitCollision - BAD TYPE!");
            break;
    }
}

// A refused message kind is not an error; a full queue is, and it names the model.

// FUNCTION: HYPERX 0x00472090
void EngineObject::PostSceneMessage(int nMessage, int nArg, int nSender, int bFromInput)
{
    char szText[0x100];

    if (m_aSceneHandlerSlot[nMessage] != 0)
    {
        if (g_pGameEngine->PostSceneMessage(m_nSlot, nMessage, nArg, nSender, bFromInput) == 0)
        {
            // STRING: HYPERX 0x004c52e8
            sprintf(szText, "Overflow of Sceneman MessagesToProcess array for model %s", m_pszName);

            g_gameApp.Fatal(szText);
        }
    }
}

// FUNCTION: HYPERX 0x00472110
void EngineObject::DispatchSceneMessage(unsigned int nMessage, int nArg, short nValue, unsigned int nSender, int bFromInput)
{
    if (m_aSceneHandlerSlot[nMessage] != 0)
    {
        if (bFromInput != 0 || m_nPhase != OBJECT_PHASE_NETWORK)
        {
            // RegisterSceneMessageHandler adds only message interactors.
            auto* const pInteractor = (MsgInteractor*)m_apSceneHandler[m_aSceneHandlerSlot[nMessage]];

            if (pInteractor->GetActiveState() != -1)
            {
                pInteractor->SetSenderAndDispatch(nArg, &nValue, nSender);
            }
        }

        if (m_pBody->pPlayer != nullptr && bFromInput == 0 && m_nPhase == OBJECT_PHASE_LOCAL)
        {
            PackAndEnqueue(nMessage, nSender, nValue, nArg);
        }
    }
}

// FUNCTION: HYPERX 0x004721a0
void EngineObject::PackAndEnqueue(unsigned int nMessage, unsigned int nSender, short nValue, int nArg)
{
    EnqueueSceneEvent(MsgEvent::PackScene(nMessage, nSender, nValue), nArg);
}

// FUNCTION: HYPERX 0x004721d0
void EngineObject::ResetSceneMessageHandlers()
{
    memset(m_aSceneHandlerSlot, 0, sizeof(m_aSceneHandlerSlot));
    memset(m_apSceneHandler, 0, sizeof(m_apSceneHandler));

    m_nSceneHandlerCount = 1;
}

// The byte table holds the low byte of the slot, so more than 255 handlers answer wrongly.

// FUNCTION: HYPERX 0x00472200
void EngineObject::RegisterSceneMessageHandler(Interactor* pHandler)
{
    const short nId = pHandler->GetNamedSlot();

    m_aSceneHandlerSlot[nId] = (unsigned char)m_nSceneHandlerCount;

    m_apSceneHandler[m_nSceneHandlerCount] = pHandler;

    const short nIndex = m_nSceneHandlerCount++;

    pHandler->m_wHandlerIndex = nIndex;
}

// FUNCTION: HYPERX 0x00472250
void EngineObject::ComputeWorldBoundingCentre(Vec3* pOut)
{
    const float flRadius = m_flBoundRadius;

    pOut->x = flRadius * m_transformB.m_a[0][2] + m_pBody->pos.x;
    pOut->y = flRadius * m_transformB.m_a[1][2] + m_pBody->pos.y;
    pOut->z = flRadius * m_transformB.m_a[2][2] + m_pBody->pos.z;
}

// FUNCTION: HYPERX 0x004722a0
void EngineObject::SetVelocity(Vec3 vel)
{
    m_pBody->vel = vel;

    m_pBody->prevPos.x = m_pBody->pos.x - vel.x;
    m_pBody->prevPos.y = m_pBody->pos.y - vel.y;
    m_pBody->prevPos.z = m_pBody->pos.z - vel.z;

    m_pBody->flSpeed = (float)sqrt(static_cast<double>(Vec3::DotInline(&vel, &vel)));

    PhysicsBody* const pBody = m_pBody;

    if (pBody->flSpeed != g_flZeroEngineObject)
    {
        pBody->velDir = vel;

        Math::Vec3Normalize(&m_pBody->velDir);
    }
}

// A shadow component id no component carries is fatal.
// FUNCTION: HYPERX 0x00472360
void EngineObject::InitShadowVertexLinks(LODNode* pLod, ModelComponent* pMesh)
{
    short                   nCount;
    const TransformObject** pLink;
    short*                  pIndex;
    int                     nLink;
    int                     nComp;

    if (m_ppShadowXforms != nullptr)
    {
        delete[] m_ppShadowXforms;
        m_ppShadowXforms = nullptr;
    }

    if (pMesh->m_shadowOffset.nBytes != 0)
    {
        nCount = pMesh->GetShadowBlock()->nVertexCount;

        m_wShadowCount   = nCount;
        pLink            = new const TransformObject*[nCount];
        m_ppShadowXforms = pLink;

        pIndex = pMesh->GetShadowBlock()->Indices();

        for (nLink = 0; nLink < m_wShadowCount; nLink++)
        {
            for (nComp = 0;; nComp++)
            {
                if (nComp >= m_wBoneCount)
                {
                    // STRING: HYPERX 0x004c5324
                    g_gameApp.Fatal("DB ERROR: Missing shadow vertex component - Model %s Comp %d!",
                                    pLod->m_pRecord->m_szName,
                                    (int)*pIndex);
                    break;
                }

                if (m_pBones[(short)nComp].m_pDesc->m_nLinkId == *pIndex)
                {
                    break;
                }
            }

            *pLink = &m_pBones[(short)nComp].m_world;

            pLink++;
            pIndex++;
        }

        m_pShadowBlock = pMesh->GetShadowBlock();

        m_pShadowPoints = pMesh->GetShadowBlock()->Vertices();

        m_pShadowFaceList = pMesh->GetShadowBlock()->aFaces;

        m_wShadowFaceCount = pMesh->GetShadowBlock()->nFaceCount;

        m_wShadowCount = pMesh->GetShadowBlock()->nVertexCount;
    }
    else
    {
        m_pShadowBlock     = nullptr;
        m_pShadowPoints    = nullptr;
        m_pShadowFaceList  = nullptr;
        m_wShadowCount     = 0;
        m_wShadowFaceCount = 0;
    }
}

// FUNCTION: HYPERX 0x004724c0
void EngineObject::CountVisibleFaces()
{
    m_wVisibleFaceTotal = 0;

    for (int iBone = 0; iBone < m_wBoneCount; iBone++)
    {
        MeshBone* const pBone = &m_pBones[(short)iBone];

        if (pBone->IsUnbound() != 0)
        {
            pBone->SetFlags(MESHBONE_HIDDEN);
        }
        else
        {
            m_wVisibleFaceTotal += pBone->GetFaceCount();
        }
    }
}

// FUNCTION: HYPERX 0x00472530
char EngineObject::InitRenderSlots(LODNode* pLod, MeshBone* pBones, CD3DTextureManager* pTexManager, char bShadowless)
{
    MeshBone* pWalk = pBones;

    for (short nSlot = 0; nSlot < m_wBoneCount; nSlot++, pWalk++)
    {
        if (bShadowless != 0)
        {
            if ((char)pWalk->BindToLODMeshShadowless(pTexManager, pLod, this, nSlot) == 0)
            {
                return 0;
            }
        }
        else
        {
            if ((char)pWalk->BindToLODMesh(pTexManager, pLod, this, nSlot) == 0)
            {
                return 0;
            }
        }

        if (pWalk->IsBound() != 0)
        {
            g_pTexManager->RescaleSoftTexCoords(nSlot, pLod, m_pModelBlock);
        }
    }

    return 1;
}

// FUNCTION: HYPERX 0x004725c0
char EngineObject::InitRenderSlotsAndShadowLinks(LODNode* pLod, MeshBone* pBones, CD3DTextureManager* pTexManager, char bShadowless)
{
    if (InitRenderSlots(pLod, pBones, pTexManager, bShadowless) != 0)
    {
        if (m_wBoneCount > 1)
        {
            MeshBone* pBone = pBones;

            for (short nSlot = 0; nSlot < m_wBoneCount; nSlot++, pBone++)
            {
                BuildShadowComponentLinks(pBone);
            }
        }

        return 1;
    }

    return 0;
}

// Applied Z, X, Y, and a zero angle is skipped, leaving the last model's sine and cosine in place.
// FUNCTION: HYPERX 0x00472620
void EngineObject::ApplyHeaderRotation(const ModelComponent* pBlob)
{
    TransformObject* pTransform;
    float            flAngle;

    if (pBlob->m_vAngle.z != g_flZeroEngineObject)
    {
        flAngle = pBlob->m_vAngle.z;

        pTransform = &m_transformA;

        pTransform->m_rotZ.flCos = (float)cos(static_cast<double>(flAngle));
        pTransform->m_rotZ.flSin = (float)sin(static_cast<double>(flAngle));

        pTransform->ApplyRotationZ();
    }

    if (pBlob->m_vAngle.x != g_flZeroEngineObject)
    {
        flAngle = pBlob->m_vAngle.x;

        pTransform = &m_transformA;

        pTransform->m_rotX.flCos = (float)cos(static_cast<double>(flAngle));
        pTransform->m_rotX.flSin = (float)sin(static_cast<double>(flAngle));

        pTransform->RotateXStaged();
    }

    if (pBlob->m_vAngle.y != g_flZeroEngineObject)
    {
        flAngle = pBlob->m_vAngle.y;

        pTransform = &m_transformA;

        pTransform->m_rotY.flCos = (float)cos(static_cast<double>(flAngle));
        pTransform->m_rotY.flSin = (float)sin(static_cast<double>(flAngle));

        pTransform->RotateAroundY();
    }
}

// FUNCTION: HYPERX 0x00474570
float EngineObject::GetDistIfFacingDefault(EngineObject* pOther)
{
    return GetDistIfFacing(pOther, g_flHalfRokObject);
}

// A dot at or below the threshold answers zero, so "behind me" and "on top of me" look alike.
// FUNCTION: HYPERX 0x00474590
float EngineObject::GetDistIfFacing(EngineObject* pOther, float flMinDot)
{
    Vec3 vDiff;

    Vec3 vPos = pOther->GetPhysicsBody()->pos;

    PhysicsBody* const pBody = m_pBody;

    vDiff.x = vPos.x - pBody->pos.x;
    vDiff.y = vPos.y - pBody->pos.y;
    vDiff.z = vPos.z - pBody->pos.z;

    const auto flDistance = (float)sqrt(static_cast<double>(vDiff.z * vDiff.z + vDiff.y * vDiff.y + vDiff.x * vDiff.x));

    Math::Vec3Normalize(&vDiff);

    Vec3* const pAxis = &m_pBody->velDir;

    if (vDiff.z * pAxis->z + vDiff.y * pAxis->y + vDiff.x * pAxis->x > flMinDot)
    {
        return flDistance;
    }

    return g_flZeroRokObject;
}

// FUNCTION: HYPERX 0x00474650
char EngineObject::EnqueueSceneEvent(int nMessage, int nArg)
{
    if (m_nSceneCount + 1 > OBJECT_RING_LENGTH)
    {
        return 0;
    }

    m_anSceneMessage[(m_nSceneHead + m_nSceneCount) % OBJECT_RING_LENGTH] = nMessage;
    m_anSceneArg[(m_nSceneHead + m_nSceneCount) % OBJECT_RING_LENGTH]     = nArg;

    m_nSceneCount++;

    return 1;
}

// FUNCTION: HYPERX 0x004746b0
char EngineObject::DequeueEvent(int* pMessage, int* pArg)
{
    if (m_nSceneCount == 0)
    {
        *pMessage = -1;
        *pArg     = -1;

        return 0;
    }

    *pMessage = m_anSceneMessage[m_nSceneHead];
    *pArg     = m_anSceneArg[m_nSceneHead];

    m_nSceneHead = (short)((m_nSceneHead + 1) % OBJECT_RING_LENGTH);
    m_nSceneCount--;

    return 1;
}

// The answer is the slot's index plus one, so nought means none had anything.
// FUNCTION: HYPERX 0x00474720
int EngineObject::PollStationaryEvents(int* pMessage, int* pArg)
{
    for (int nIndex = 0; nIndex < g_nStationaryCount; nIndex++)
    {
        if (g_pGameEngine->m_ppSlots[(short)g_aStationarySlots[nIndex]]->DequeueEvent(pMessage, pArg) != 0)
        {
            return nIndex + 1;
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x00474780
char EngineObject::EnqueueInputCommand(int nCommand, int nArg)
{
    while (m_nCommandCount + 1 > OBJECT_RING_LENGTH)
    {
        m_nCommandCount--;
        m_nCommandHead = (short)((m_nCommandHead + 1) % OBJECT_RING_LENGTH);
    }

    m_anCommand[(m_nCommandHead + m_nCommandCount) % OBJECT_RING_LENGTH]    = nCommand;
    m_anCommandArg[(m_nCommandHead + m_nCommandCount) % OBJECT_RING_LENGTH] = nArg;

    m_nCommandCount++;

    return 1;
}

// FUNCTION: HYPERX 0x00474810
char EngineObject::DequeueInputCommand(int* pCommand, int* pArg)
{
    if (m_nCommandCount == 0)
    {
        *pCommand = -1;
        *pArg     = -1;

        return 0;
    }

    *pCommand = m_anCommand[m_nCommandHead];
    *pArg     = m_anCommandArg[m_nCommandHead];

    m_nCommandHead = (short)((m_nCommandHead + 1) % OBJECT_RING_LENGTH);
    m_nCommandCount--;

    return 1;
}
