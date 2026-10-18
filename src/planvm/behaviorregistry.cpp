#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "globals.h"
#include "planvmglobals.h"
#include "common/pooledownerroot.h"
#include "common/behobjglidetoobjectpool.h"
#include "common/behobjglidetopositionpool.h"
#include "common/behobjglidebetweenfacingpool.h"
#include "common/behobjavoidancepool.h"
#include "common/behobjupdaterelativepospool.h"
#include "common/behobjglidepool.h"
#include "common/behobjglidelclpool.h"
#include "common/behobjrailslidepool.h"
#include "common/behobjflypool.h"
#include "common/behobjflylclpool.h"
#include "common/behobjflytowardpool.h"
#include "common/behobjmonitorkeyboardpool.h"
#include "common/behobjupdatewhileattachedpool.h"
#include "common/behobjsetinplacepool.h"
#include "common/behobjsetflatinplacepool.h"
#include "common/behobjsetmovinginplacepool.h"
#include "common/behobjdonothingpool.h"
#include "common/behobjopenandclosepool.h"
#include "common/behobjtrackingpool.h"
#include "common/behobjheadtrackingpool.h"
#include "common/behobjwatchingpool.h"
#include "common/behobjtracktrajectorypool.h"
#include "common/behobjtrackpasspreppool.h"
#include "common/behobjcoachteampool.h"
#include "common/behobjcandytrackingpool.h"
#include "engine/devicector.h"
#include "behparam.h"
#include "plannode.h"
#include "engine/engineobject.h"
#include "planvm/behaviorregistry.h"

// Runs each pool's Destroy slot and stores its answer back, stopping at the first null.

// FUNCTION: HYPERX 0x0041c650
void BehaviorRegistry::DestroyAll()
{
    short iSlot = 0;

    auto* pNode = g_apBehaviorRegistry[0];

    while (pNode != nullptr)
    {
        g_apBehaviorRegistry[iSlot] = pNode->Destroy();

        iSlot++;

        pNode = g_apBehaviorRegistry[iSlot];
    }
}

// FUNCTION: HYPERX 0x0041c690
void BehaviorRegistry::InitAll()
{
    short iSlot = 0;

    auto* pNode = g_apBehaviorRegistry[0];

    while (pNode != nullptr)
    {
        iSlot++;

        pNode->Init();

        pNode = g_apBehaviorRegistry[iSlot];
    }
}

// GLOBAL: HYPERX 0x004cb508
extern BehParamHandlerFn g_apfnBehaviorParamHandlers[kSetBehaviorParamCount];

// Called once by GameEngine::InitPlanSystems; the registry ends at its first null slot.
// FUNCTION: HYPERX 0x0041cdd0
void BehaviorRegistry::Init()
{
    g_apfnBehaviorParamHandlers[kSetModsTweenNFacing]    = BehParam::SetModsTweenNFacing;
    g_apfnBehaviorParamHandlers[kSetKeyMonitorMode]      = BehParam::SetKeyMonitorMode;
    g_apfnBehaviorParamHandlers[kSetInteractorLatency]   = BehParam::SetInteractorLatency;
    g_apfnBehaviorParamHandlers[kSetTarget]              = BehParam::SetTarget;
    g_apfnBehaviorParamHandlers[kSetTargetPosition]      = BehParam::SetTargetPosition;
    g_apfnBehaviorParamHandlers[kSetRelativePosition]    = BehParam::SetRelativePosition;
    g_apfnBehaviorParamHandlers[kSetModelImRelativeTo]   = BehParam::SetModelImRelativeTo;
    g_apfnBehaviorParamHandlers[kSetObjectToAvoid]       = BehParam::SetObjectToAvoid;
    g_apfnBehaviorParamHandlers[kSetModelImFacing]       = BehParam::SetModelImFacing;
    g_apfnBehaviorParamHandlers[kSetDigressive]          = BehParam::SetDigressive;
    g_apfnBehaviorParamHandlers[kSetPurgeFlag]           = BehParam::SetPurgeFlag;
    g_apfnBehaviorParamHandlers[kSetPersistentAnimation] = BehParam::SetPersistentAnimation;
    g_apfnBehaviorParamHandlers[kSetAnimation]           = BehParam::SetAnimation;
    g_apfnBehaviorParamHandlers[kSetAnimationEvent]      = BehParam::SetAnimationEvent;
    g_apfnBehaviorParamHandlers[kSetTargetGoal]          = BehParam::SetTargetGoal;
    g_apfnBehaviorParamHandlers[kSetObjectToFollow]      = BehParam::SetObjectToFollow;

    g_apBehaviorRegistry[kBehGlideToObject] = new BehObjGlideToObjectPool;

    g_apfnBehaviorParamHandlers[kSetDoubleScore]  = BehParam::SetTargetGoal;
    g_apBehaviorRegistry[kBehGlideToPosition]     = new BehObjGlideToPositionPool;
    g_apBehaviorRegistry[kBehGlideBetweenFacing]  = new BehObjGlideBetweenFacingPool;
    g_apBehaviorRegistry[kBehAvoidance]           = new BehObjAvoidancePool;
    g_apBehaviorRegistry[kBehUpdateRelativePos]   = new BehObjUpdateRelativePosPool;
    g_apBehaviorRegistry[kBehGlide]               = new BehObjGlidePool;
    g_apBehaviorRegistry[kBehGlideLcl]            = new BehObjGlideLCLPool;
    g_apBehaviorRegistry[kBehRailSlide]           = new BehObjRailSlidePool;
    g_apBehaviorRegistry[kBehFly]                 = new BehObjFlyPool;
    g_apBehaviorRegistry[kBehFlyLcl]              = new BehObjFlyLCLPool;
    g_apBehaviorRegistry[kBehFlyToward]           = new BehObjFlyTowardPool;
    g_apBehaviorRegistry[kBehMonitorKeyboard]     = new BehObjMonitorKeyboardPool;
    g_apBehaviorRegistry[kBehUpdateWhileAttached] = new BehObjUpdateWhileAttachedPool;
    g_apBehaviorRegistry[kBehSetInPlace]          = new BehObjSetInPlacePool;
    g_apBehaviorRegistry[kBehSetFlatInPlace]      = new BehObjSetFlatInPlacePool;
    g_apBehaviorRegistry[kBehSetMovingInPlace]    = new BehObjSetMovingInPlacePool;
    g_apBehaviorRegistry[kBehDoNothing]           = new BehObjDoNothingPool;
    g_apBehaviorRegistry[kBehOpenAndClose]        = new BehObjOpenAndClosePool;
    g_apBehaviorRegistry[kBehTracking]            = new BehObjTrackingPool;
    g_apBehaviorRegistry[kBehHeadTracking]        = new BehObjHeadTrackingPool;
    g_apBehaviorRegistry[kBehWatching]            = new BehObjWatchingPool;
    g_apBehaviorRegistry[kBehTrackTrajectory]     = new BehObjTrackTrajectoryPool;
    g_apBehaviorRegistry[kBehTrackPassPrep]       = new BehObjTrackPassPrepPool;
    g_apBehaviorRegistry[kBehCoachTeam]           = new BehObjCoachTeamPool;
    g_apBehaviorRegistry[kBehCandyTracking]       = new BehObjCandyTrackingPool;
    g_apBehaviorRegistry[kBehCount]               = nullptr;
}
