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

    PooledOwnerRoot* pNode = g_apBehaviorRegistry[0];

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

    PooledOwnerRoot* pNode = g_apBehaviorRegistry[0];

    while (pNode != nullptr)
    {
        iSlot++;

        pNode->Init();

        pNode = g_apBehaviorRegistry[iSlot];
    }
}

// GLOBAL: HYPERX 0x004cb508
extern BehParamHandlerFn g_apfnBehaviorParamHandlers[SET_BEHAVIOR_PARAM_COUNT];

// Called once by GameEngine::InitPlanSystems; the registry ends at its first null slot.
// FUNCTION: HYPERX 0x0041cdd0
void BehaviorRegistry::Init()
{
    g_apfnBehaviorParamHandlers[SET_MODS_TWEEN_N_FACING]  = BehParam::SetModsTweenNFacing;
    g_apfnBehaviorParamHandlers[SET_KEY_MONITOR_MODE]     = BehParam::SetKeyMonitorMode;
    g_apfnBehaviorParamHandlers[SET_INTERACTOR_LATENCY]   = BehParam::SetInteractorLatency;
    g_apfnBehaviorParamHandlers[SET_TARGET]               = BehParam::SetTarget;
    g_apfnBehaviorParamHandlers[SET_TARGET_POSITION]      = BehParam::SetTargetPosition;
    g_apfnBehaviorParamHandlers[SET_RELATIVE_POSITION]    = BehParam::SetRelativePosition;
    g_apfnBehaviorParamHandlers[SET_MODEL_IM_RELATIVE_TO] = BehParam::SetModelImRelativeTo;
    g_apfnBehaviorParamHandlers[SET_OBJECT_TO_AVOID]      = BehParam::SetObjectToAvoid;
    g_apfnBehaviorParamHandlers[SET_MODEL_IM_FACING]      = BehParam::SetModelImFacing;
    g_apfnBehaviorParamHandlers[SET_DIGRESSIVE]           = BehParam::SetDigressive;
    g_apfnBehaviorParamHandlers[SET_PURGE_FLAG]           = BehParam::SetPurgeFlag;
    g_apfnBehaviorParamHandlers[SET_PERSISTENT_ANIMATION] = BehParam::SetPersistentAnimation;
    g_apfnBehaviorParamHandlers[SET_ANIMATION]            = BehParam::SetAnimation;
    g_apfnBehaviorParamHandlers[SET_ANIMATION_EVENT]      = BehParam::SetAnimationEvent;
    g_apfnBehaviorParamHandlers[SET_TARGET_GOAL]          = BehParam::SetTargetGoal;
    g_apfnBehaviorParamHandlers[SET_OBJECT_TO_FOLLOW]     = BehParam::SetObjectToFollow;

    g_apBehaviorRegistry[BEH_GLIDE_TO_OBJECT] = new BehObjGlideToObjectPool;

    g_apfnBehaviorParamHandlers[SET_DOUBLE_SCORE]   = BehParam::SetTargetGoal;
    g_apBehaviorRegistry[BEH_GLIDE_TO_POSITION]     = new BehObjGlideToPositionPool;
    g_apBehaviorRegistry[BEH_GLIDE_BETWEEN_FACING]  = new BehObjGlideBetweenFacingPool;
    g_apBehaviorRegistry[BEH_AVOIDANCE]             = new BehObjAvoidancePool;
    g_apBehaviorRegistry[BEH_UPDATE_RELATIVE_POS]   = new BehObjUpdateRelativePosPool;
    g_apBehaviorRegistry[BEH_GLIDE]                 = new BehObjGlidePool;
    g_apBehaviorRegistry[BEH_GLIDE_LCL]             = new BehObjGlideLCLPool;
    g_apBehaviorRegistry[BEH_RAIL_SLIDE]            = new BehObjRailSlidePool;
    g_apBehaviorRegistry[BEH_FLY]                   = new BehObjFlyPool;
    g_apBehaviorRegistry[BEH_FLY_LCL]               = new BehObjFlyLCLPool;
    g_apBehaviorRegistry[BEH_FLY_TOWARD]            = new BehObjFlyTowardPool;
    g_apBehaviorRegistry[BEH_MONITOR_KEYBOARD]      = new BehObjMonitorKeyboardPool;
    g_apBehaviorRegistry[BEH_UPDATE_WHILE_ATTACHED] = new BehObjUpdateWhileAttachedPool;
    g_apBehaviorRegistry[BEH_SET_IN_PLACE]          = new BehObjSetInPlacePool;
    g_apBehaviorRegistry[BEH_SET_FLAT_IN_PLACE]     = new BehObjSetFlatInPlacePool;
    g_apBehaviorRegistry[BEH_SET_MOVING_IN_PLACE]   = new BehObjSetMovingInPlacePool;
    g_apBehaviorRegistry[BEH_DO_NOTHING]            = new BehObjDoNothingPool;
    g_apBehaviorRegistry[BEH_OPEN_AND_CLOSE]        = new BehObjOpenAndClosePool;
    g_apBehaviorRegistry[BEH_TRACKING]              = new BehObjTrackingPool;
    g_apBehaviorRegistry[BEH_HEAD_TRACKING]         = new BehObjHeadTrackingPool;
    g_apBehaviorRegistry[BEH_WATCHING]              = new BehObjWatchingPool;
    g_apBehaviorRegistry[BEH_TRACK_TRAJECTORY]      = new BehObjTrackTrajectoryPool;
    g_apBehaviorRegistry[BEH_TRACK_PASS_PREP]       = new BehObjTrackPassPrepPool;
    g_apBehaviorRegistry[BEH_COACH_TEAM]            = new BehObjCoachTeamPool;
    g_apBehaviorRegistry[BEH_CANDY_TRACKING]        = new BehObjCandyTrackingPool;
    g_apBehaviorRegistry[BEH_COUNT]                 = nullptr;
}
