#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "globals.h"
#include "planvmglobals.h"
#include "common/behaviorpools.h"
#include "engine/devicector.h"
#include "behparam.h"
#include "plannode.h"
#include "engine/engineobject.h"
#include "planvm/behaviorregistry.h"

// Destroys each pool, stopping at the first null.

// FUNCTION: HYPERX 0x0041c650
void BehaviorRegistry::DestroyAll()
{
    short iSlot = 0;

    while (g_apBehaviorRegistry[iSlot] != nullptr)
    {
        g_apBehaviorRegistry[iSlot].reset();

        iSlot++;
    }
}

// FUNCTION: HYPERX 0x0041c690
void BehaviorRegistry::InitAll()
{
    short iSlot = 0;

    auto* pNode = g_apBehaviorRegistry[0].get();

    while (pNode != nullptr)
    {
        iSlot++;

        pNode->Init();

        pNode = g_apBehaviorRegistry[iSlot].get();
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

    g_apBehaviorRegistry[kBehGlideToObject] = std::make_unique<BehObjGlideToObjectPool>();

    g_apfnBehaviorParamHandlers[kSetDoubleScore]  = BehParam::SetTargetGoal;
    g_apBehaviorRegistry[kBehGlideToPosition]     = std::make_unique<BehObjGlideToPositionPool>();
    g_apBehaviorRegistry[kBehGlideBetweenFacing]  = std::make_unique<BehObjGlideBetweenFacingPool>();
    g_apBehaviorRegistry[kBehAvoidance]           = std::make_unique<BehObjAvoidancePool>();
    g_apBehaviorRegistry[kBehUpdateRelativePos]   = std::make_unique<BehObjUpdateRelativePosPool>();
    g_apBehaviorRegistry[kBehGlide]               = std::make_unique<BehObjGlidePool>();
    g_apBehaviorRegistry[kBehGlideLcl]            = std::make_unique<BehObjGlideLCLPool>();
    g_apBehaviorRegistry[kBehRailSlide]           = std::make_unique<BehObjRailSlidePool>();
    g_apBehaviorRegistry[kBehFly]                 = std::make_unique<BehObjFlyPool>();
    g_apBehaviorRegistry[kBehFlyLcl]              = std::make_unique<BehObjFlyLCLPool>();
    g_apBehaviorRegistry[kBehFlyToward]           = std::make_unique<BehObjFlyTowardPool>();
    g_apBehaviorRegistry[kBehMonitorKeyboard]     = std::make_unique<BehObjMonitorKeyboardPool>();
    g_apBehaviorRegistry[kBehUpdateWhileAttached] = std::make_unique<BehObjUpdateWhileAttachedPool>();
    g_apBehaviorRegistry[kBehSetInPlace]          = std::make_unique<BehObjSetInPlacePool>();
    g_apBehaviorRegistry[kBehSetFlatInPlace]      = std::make_unique<BehObjSetFlatInPlacePool>();
    g_apBehaviorRegistry[kBehSetMovingInPlace]    = std::make_unique<BehObjSetMovingInPlacePool>();
    g_apBehaviorRegistry[kBehDoNothing]           = std::make_unique<BehObjDoNothingPool>();
    g_apBehaviorRegistry[kBehOpenAndClose]        = std::make_unique<BehObjOpenAndClosePool>();
    g_apBehaviorRegistry[kBehTracking]            = std::make_unique<BehObjTrackingPool>();
    g_apBehaviorRegistry[kBehHeadTracking]        = std::make_unique<BehObjHeadTrackingPool>();
    g_apBehaviorRegistry[kBehWatching]            = std::make_unique<BehObjWatchingPool>();
    g_apBehaviorRegistry[kBehTrackTrajectory]     = std::make_unique<BehObjTrackTrajectoryPool>();
    g_apBehaviorRegistry[kBehTrackPassPrep]       = std::make_unique<BehObjTrackPassPrepPool>();
    g_apBehaviorRegistry[kBehCoachTeam]           = std::make_unique<BehObjCoachTeamPool>();
    g_apBehaviorRegistry[kBehCandyTracking]       = std::make_unique<BehObjCandyTrackingPool>();
    g_apBehaviorRegistry[kBehCount]               = nullptr;
}
