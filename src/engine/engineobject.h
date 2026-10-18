#ifndef HYPERBLADE_ENGINEOBJECT_H
#define HYPERBLADE_ENGINEOBJECT_H

#include "common/compat.h"
#include "engine/objectbase.h"
#include "engine/syncsendcursor.h"
#include "math/transformobject.h"
#include <mutex>

class AnimDB;
class AnimPlayer;
class BoundBubble;
class BubbleCmd;
class CPlayer;
class CameraObject;
class CollisionPlane;
class LODNode;
struct MaterialListNode;
class MeshBone;
struct ModelComponent;
class ObjectDef;
struct PhysicsBody;
class PlanState;
class TransformObject;
struct Vec3;

// The scene-message registry: one byte per message id, one handler per slot.
constexpr auto kSceneMessageCount = 0x8e;
constexpr auto kSceneHandlerSlots = 50;

// The length of the scene event ring and of the input command ring.
constexpr auto kObjectRingLength = 0x96;

// The bits of EngineObject::m_nFlags. An unculled object skips the camera's frustum test, one with
// no model has nothing to release, a hidden one is neither drawn nor body checked, and the posed bit
// says the render slots were transformed this frame and is cleared before the next.
constexpr auto kEngineObjectNoCull  = 0x01;
constexpr auto kEngineObjectNoModel = 0x04;
constexpr auto kEngineObjectHidden  = 0x10;
constexpr auto kEngineObjectPosed   = 0x20;

// Input commands that are not a scene message: the plan in the high word and the
// behaviour in the low, or the behaviour alone.
constexpr auto kCommandSetPlanAndBehavior = (-2);
constexpr auto kCommandSetBehavior        = (-3);

// 0xd14 bytes, where every derived class puts its first field; offsets past it are per class.
// VTABLE: HYPERX 0x004b38f0
class EngineObject : public ObjectBase
{
public:
    EngineObject(int nSlot, class ObjectDef* pDef);

    static float ComputeDistance(EngineObject* pFirst, EngineObject* pSecond);
    static int   GetApproachAngleSector(EngineObject* pSelf, EngineObject* pOther);
    static int   IsRokCarrier(EngineObject* pActor);
    static void  PostCollisionCommentary(EngineObject* pA, EngineObject* pB);
    char         ProcessReceivedPacket();
    // Reached on the object itself by the node list dialog.
    short GetBehaviorIndex(class PlanState* pState, const char* pszName);

    void CountVisibleFaces();

    ~EngineObject() override;

    // The engine slot, not the kind; every class shares this body. The kind is m_nKind.
    short GetSlot() override;

    // The whole of m_nSlot, which GetSlot narrows to a short.
    int GetSlotWord() const
    {
        return m_nSlot;
    }

    PhysicsBody*     GetPhysicsBody() override;
    TransformObject* GetLocalTransformPtr() override;
    // Also the head of the object's agenda item list.
    class AgendaItem*     GetObserver() override;
    void                  SetObserver(class AgendaItem* pObserver) override;
    void                  PostSceneMessage(int nMessage, int nArg, int nSender, int bFromInput) override;
    void                  PostEvent(int nMessage, int nArg, int nSender) override;
    void                  PostSceneEvent(int nMessage, int nArg) override;
    void                  DispatchSceneMessage(unsigned int nMessage, int nArg, short nValue, unsigned int nSender, int bFromInput) override;
    void                  StartClipPlayback(class AnimDB* pClip, float flRate, int nFlags) override;
    void                  BindAnimEventChannel(int nChannel, int nTarget, int nKind, int nParam) override;
    void                  ResetSceneMessageHandlers() override;
    class PlanState*      GetPlanState() override;
    void                  RegisterSceneMessageHandler(class Interactor* pHandler) override;
    class CPlayer*        GetPhysStateObj() override;
    std::recursive_mutex* GetCPlayerStateLock() override;
    void                  InstantiateAIPlan(short nIndex) override;
    ObjectPhase           GetPhase() override;
    char                  EnqueueSceneEvent(int nMessage, int nArg) override;
    short                 FindPlanByName(const char* pszName) override;
    Vec3*                 GetWorldPos(Vec3* pOut) override;
    struct Vec3*          GetPositionPtr() override;
    char                  IsVisible() override;
    void                  SetVisible(char bFlag) override;
    void                  ApplyRenderSlotPose() override;

    // Slots twenty six to thirty one, which the base's table does not have.
    // The original folds this with every bare `ret` at 0x00416e30, so it carries no marker.
    virtual void Postpass()
    {
    }

    virtual void  SetEntitySlot(int nEntitySlot);
    virtual void  DrawTransformedMesh(class CollisionShape* pSurface);
    void          SubmitShadowToHardware() const;
    virtual short GetShadowCount();
    virtual short GetComponentCount();
    virtual void  UpdateRenderSlotTransform(short nSlotIdx);

    // Picks each component's level of detail and pushes what survives onto the render stack.
    void UpdateLODAndPushComponents(class CameraObject* pCamera);

    // Computes m_vCentre and m_flBoundRadius.
    void ComputeBoundingSphere();

    void InitFields();
    void SelectAndActivatePlan();
    char InitModel(int nSlot, class ObjectDef* pDef);
    void InitCollisionBubble(class BubbleCmd* pShape);

    // The engine's input pump calls it per object per frame.
    char DequeueInputCommand(int* pCommand, int* pArg);

    void  SetVelocity(Vec3 vel);
    float GetDistIfFacing(EngineObject* pOther, float flMinDot);
    // The same at the default half cosine, which is the only threshold any caller asks for.
    float GetDistIfFacingDefault(EngineObject* pOther);
    void  ComputeWorldBoundingCentre(Vec3* pOut);
    void  PackAndEnqueue(unsigned int nMessage, unsigned int nSender, short nValue, int nArg);
    void  ApplyHeaderRotation(const struct ModelComponent* pBlob);
    // Stores nSlot straight into m_nSlot.
    char ReInitModel(int nSlot, class ObjectDef* pDef);
    void UpdateCoachAI();
    // What the coach checks of every other object CCheckAI runs on: nothing.
    void CheckObjectAI(EngineObject* pObject);

    // The render slots this object's model is bound into.
    char InitRenderSlots(class LODNode* pLod, class MeshBone* pBones, class CD3DTextureManager* pTexManager, char bShadowless);
    char InitRenderSlotsAndShadowLinks(class LODNode* pLod, class MeshBone* pBones, class CD3DTextureManager* pTexManager, char bShadowless);
    void InitShadowVertexLinks(class LODNode* pLod, struct ModelComponent* pMesh);
    void BuildShadowComponentLinks(class MeshBone* pSlot);

    // The tick of the last goal, for the score debounce; zero means no goal yet.
    unsigned int m_nLastGoalTickMs; // 0x008
protected:
    // Three words the constructor zeroes and nothing reads.
    int m_nUnusedA; // 0x00c
    int m_nUnusedB; // 0x010
    int m_nUnusedC; // 0x014
    // The object's slot in the engine table; its low bit is the team.
public:
    int m_nSlot; // 0x018

    char* m_pszName; // 0x01c
    int   m_nKind;   // 0x020, the value the per-frame tick switches on

protected:
    // The same record LODNode::m_pRecord points at, copied here when the model is bound.
    struct ModelComponent* m_pMeshBlob; // 0x024
public:
    class LODNode* m_pLod; // 0x028
    // Public because src/physics/boundbubble.cpp reads the field itself.
    short m_wBoneCount; // 0x02c

    const short& GetBoneCount() const
    {
        return m_wBoneCount;
    }

    short m_wLodLevel; // 0x02e
protected:
    float m_flLodDistSq; // 0x030
public:
    class AnimPlayer* m_pAnimPlayer; // 0x034

    struct MaterialListNode* m_pModelBlock; // 0x038
protected:
    char                m_cUnused;       // 0x03c
    struct ShadowBlock* m_pShadowBlock;  // 0x040
    const Vec3*         m_pShadowPoints; // 0x044
    // One bone transform per shadow vertex, owned by this object and m_wShadowCount long.
    const TransformObject** m_ppShadowXforms; // 0x048
    // Some macros read these words as a vertex and face count; most read them as the shadow counts.
    short              m_wShadowCount;     // 0x04c
    struct ShadowFace* m_pShadowFaceList;  // 0x050
    short              m_wShadowFaceCount; // 0x054
    // The bounding sphere ComputeBoundingSphere builds.
public:
    float m_flBoundRadius; // 0x058
protected:
    Vec3  m_vCentre;  // 0x05c
    short m_wUnused;  // 0x068
    Vec3  m_vUnusedA; // 0x06c, zeroed by InitFields; nothing in this tree reads it
    // Only InitFields writes it, with zero, so every pose runs on slot zero.
    short m_wRenderSlot; // 0x078
    Vec3  m_vUnusedB;    // 0x07c, zeroed by InitFields; nothing in this tree reads it
    Vec3  m_vUnusedC;    // 0x088, zeroed by InitFields; nothing in this tree reads it
public:
    class MeshBone* m_pBones; // 0x094
    void            AttachBubble(short nBone, BoundBubble* pBubble);

    // Public because src/physics/physicsbody.cpp reads one column of the second matrix.
    TransformObject m_transformA; // 0x098
    TransformObject m_transformB; // 0x114
    TransformObject m_transformC; // 0x190

protected:
    // InitFields clears it and the model loaders set it.
    char         m_bScaledBounds; // 0x20c
    BoundBubble* m_pBubble;       // 0x210
public:
    ObjectPhase m_nPhase; // 0x214
protected:
    ObjectPhase m_nUnusedPhase; // 0x218, written with the phase and never read
    // The camera's cull result each frame; the model loaders set it.
    char m_bVisible; // 0x21c
public:
    // Compiles every plan the object owns, one PlanState per source.
    void CompileAllPlans();

    unsigned char HasNoModel() const
    {
        return static_cast<unsigned char>(m_nFlags >> 2) & 1;
    }

    unsigned char IsHidden() const
    {
        return static_cast<unsigned char>(m_nFlags >> 4) & 1;
    }

    unsigned int m_nFlags; // 0x220

    PhysicsBody* m_pBody; // 0x224

    // A PlanState list head.
    // Public: src/planvm/planeditordialog.cpp writes it.
    class PlanState* m_pPlanState; // 0x228
    // Public: src/engine/level.cpp reads it from a free function.
    class PlanState* m_pPlanStates; // 0x22c
protected:
    // One plan blob per plan, null terminated.
    union PlanWord** m_ppPlanBlobs; // 0x230
public:
    // The plan names, taken straight off ObjectDef::m_apszPlanNames.
    char** m_papszPlanNames; // 0x234

    const char* GetPlanName(short iPlan) const
    {
        return m_papszPlanNames[iPlan];
    }

    short m_nPlanCount; // 0x238, ObjectDef::m_nPlanCount copied
    // The scene-message registry: a byte table by message id, a pointer table by slot, and a count.
    unsigned char m_aSceneHandlerSlot[kSceneMessageCount]; // 0x23a
protected:
    class Interactor* m_apSceneHandler[kSceneHandlerSlots]; // 0x2c8

    short m_nSceneHandlerCount; // 0x390
    int   m_nTypeId;            // 0x394
    // The slot most recently associated with this object: the Rok's carrier, a spectator's target.
public:
    int m_nEntitySlot; // 0x398

    // The tick copies the collision sub's sync word into the first, last frame's into the second.
    short m_wSyncedPlan;      // 0x39c
    short m_wSyncedBehaviour; // 0x39e

    // Two rings of kObjectRingLength entries: the scene events and the input commands.
    short m_nSceneCount;                       // 0x3a0
    short m_nSceneHead;                        // 0x3a2
    int   m_anSceneMessage[kObjectRingLength]; // 0x3a4
    int   m_anSceneArg[kObjectRingLength];     // 0x5fc
    short m_nCommandCount;                     // 0x854
    short m_nCommandHead;                      // 0x856
    int   m_anCommand[kObjectRingLength];      // 0x858
    int   m_anCommandArg[kObjectRingLength];   // 0xab0

    // What CountVisibleFaces accumulates: the faces of every bone it does not hide.
    short m_wVisibleFaceTotal; // 0xd08
protected:
    // The tick the last periodic message went out; RokObject and PlayerObject share it.
    int m_nLastSendMs; // 0xd0c
    // The stat row wraps past kBotSlotLimit.
    SyncSendCursor m_sendCursor; // 0xd10

public:
    // The dequeue halves of the two rings above.
    char DequeueEvent(int* pMessage, int* pArg);
    int  PollStationaryEvents(int* pMessage, int* pArg);
    // The packet reader calls it four times on the object it has just moved.
    char EnqueueInputCommand(int nCommand, int nArg);

    void ApplySyncedBehaviour(class CPlayer* pOwner, const struct NetMoveBody* pBody, short* pOutPlan, short* pOutBehaviour);

    static void StoreVector(Vec3* pDest, float flX, float flY, float flZ);

    static void FreeName(char*& pszName);

    static void CoachUpdate(class EngineObject* pActor);
};

// SYNTHETIC: HYPERX 0x00472720
// EngineObject::`scalar deleting destructor'

// The atexit thunk for UpdateRenderSlotTransform's scratch transform.
// SYNTHETIC: HYPERX 0x004716e0
// $E14

#endif
