#ifndef HYPERBLADE_OBJECTBASE_H
#define HYPERBLADE_OBJECTBASE_H

#include "common/compat.h"
#include "engine/objectphase.h"
#include <mutex>

class AnimDB;
struct PhysicsBody;
class PlanState;
class TransformObject;
struct Vec3;

// Its own constructor clears it ahead of the owner's vptr store, as the client does.
struct InstallCost
{
    InstallCost()
    {
        nLevels = 0;
    }

    int nLevels;
};

// The root of the game-object family, abstract: a destructor and 25 _purecall slots.
// VTABLE: HYPERX 0x004b3970
class ObjectBase
{
public:
    // FUNCTION: HYPERX 0x004726f0
    virtual ~ObjectBase()
    {
    }

    virtual short             GetSlot()                                                                                                 = 0;
    virtual PhysicsBody*      GetPhysicsBody()                                                                                          = 0;
    virtual TransformObject*  GetLocalTransformPtr()                                                                                    = 0;
    virtual class AgendaItem* GetObserver()                                                                                             = 0;
    virtual void              SetObserver(class AgendaItem* pObserver)                                                                  = 0;
    virtual void              PostSceneMessage(int nMessage, int nArg, int nSender, int bFromInput)                                     = 0;
    virtual void              PostEvent(int nMessage, int nArg, int nSender)                                                            = 0;
    virtual void              PostSceneEvent(int nMessage, int nArg)                                                                    = 0;
    virtual void              DispatchSceneMessage(unsigned int nMessage, int nArg, short nValue, unsigned int nSender, int bFromInput) = 0;
    virtual void              StartClipPlayback(class AnimDB* pClip, float flRate, int nFlags)                                          = 0;
    virtual void              BindAnimEventChannel(int nChannel, int nTarget, int nKind, int nParam)                                    = 0;
    virtual void              ResetSceneMessageHandlers()                                                                               = 0;
    // Callers read the result four ways; the true type is unknown.
    virtual class PlanState*      GetPlanState()                                          = 0;
    virtual void                  RegisterSceneMessageHandler(class Interactor* pHandler) = 0;
    virtual class CPlayer*        GetPhysStateObj()                                       = 0;
    virtual std::recursive_mutex* GetCPlayerStateLock()                                   = 0;
    virtual void                  InstantiateAIPlan(short nIndex)                         = 0;
    virtual ObjectPhase           GetPhase()                                              = 0;
    virtual char                  EnqueueSceneEvent(int nMessage, int nArg)               = 0;
    virtual short                 FindPlanByName(const char* pszName)                     = 0;
    virtual Vec3*                 GetWorldPos(Vec3* pOut)                                 = 0;
    virtual struct Vec3*          GetPositionPtr()                                        = 0;
    virtual char                  IsVisible()                                             = 0;
    virtual void                  SetVisible(char bFlag)                                  = 0;
    virtual void                  ApplyRenderSlotPose()                                   = 0;

    // What GameApp::GetInstallLevel moved by while GameEngine::CreateObject built this object.
    InstallCost m_installCost; // 0x004, the vtable slot at 0x000 being the compiler's
};

// SYNTHETIC: HYPERX 0x00472700
// ObjectBase::`scalar deleting destructor'

#endif
