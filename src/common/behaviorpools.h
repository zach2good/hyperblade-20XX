#ifndef HYPERBLADE_BEHAVIORPOOLS_H
#define HYPERBLADE_BEHAVIORPOOLS_H

#include "common/nodepool.h"
#include "common/poolcounts.h"
#include "common/pooledclasses.h"

template <typename T, auto& Count>
using BehaviorPool = NodePool<GameBehaviorObject, &GameBehaviorObject::m_pStandby, T, Count>;

// VTABLE: HYPERX 0x004b258c
using BehObjAvoidancePool = BehaviorPool<BehObj_Avoidance, g_wPoolCountBehObjAvoidance>;

// VTABLE: HYPERX 0x004b23e8
using BehObjCandyTrackingPool = BehaviorPool<BehObj_CandyTracking, g_nPoolCountBehObjCandyTracking>;

// VTABLE: HYPERX 0x004b23fc
using BehObjCoachTeamPool = BehaviorPool<BehObj_CoachTeam, g_nPoolCountBehObjCoachTeam>;

// VTABLE: HYPERX 0x004b2488
using BehObjDoNothingPool = BehaviorPool<BehObj_DoNothing, g_wPoolCountBehObjDoNothing>;

// VTABLE: HYPERX 0x004b2514
using BehObjFlyLCLPool = BehaviorPool<BehObj_FlyLCL, g_wPoolCountBehObjFlyLCL>;

// VTABLE: HYPERX 0x004b2528
using BehObjFlyPool = BehaviorPool<BehObj_Fly, g_nPoolCountBehObjFly>;

// VTABLE: HYPERX 0x004b2500
using BehObjFlyTowardPool = BehaviorPool<BehObj_FlyToward, g_nPoolCountBehObjFlyToward>;

// VTABLE: HYPERX 0x004b25a0
using BehObjGlideBetweenFacingPool = BehaviorPool<BehObj_GlideBetweenFacing, g_nPoolCountBehObjGlideBetweenFacing>;

// VTABLE: HYPERX 0x004b2550
using BehObjGlideLCLPool = BehaviorPool<BehObj_GlideLCL, g_nPoolCountBehObjGlideLCL>;

// VTABLE: HYPERX 0x004b2564
using BehObjGlidePool = BehaviorPool<BehObj_Glide, g_wPoolCountBehObjGlide>;

// VTABLE: HYPERX 0x004b25c8
using BehObjGlideToObjectPool = BehaviorPool<BehObj_GlideToObject, g_nPoolCountBehObjGlideToObject>;

// VTABLE: HYPERX 0x004b25b4
using BehObjGlideToPositionPool = BehaviorPool<BehObj_GlideToPosition, g_wPoolCountBehObjGlideToPosition>;

// VTABLE: HYPERX 0x004b244c
using BehObjHeadTrackingPool = BehaviorPool<BehObj_HeadTracking, g_nPoolCountBehObjHeadTracking>;

// VTABLE: HYPERX 0x004b24ec
using BehObjMonitorKeyboardPool = BehaviorPool<BehObj_MonitorKeyboard, g_wPoolCountBehObjMonitorKeyboard>;

// VTABLE: HYPERX 0x004b2474
using BehObjOpenAndClosePool = BehaviorPool<BehObj_OpenAndClose, g_nPoolCountBehObjOpenAndClose>;

// VTABLE: HYPERX 0x004b253c
using BehObjRailSlidePool = BehaviorPool<BehObj_RailSlide, g_wPoolCountBehObjRailSlide>;

// VTABLE: HYPERX 0x004b24b0
using BehObjSetFlatInPlacePool = BehaviorPool<BehObj_SetFlatInPlace, g_wPoolCountBehObjSetFlatInPlace>;

// VTABLE: HYPERX 0x004b24c4
using BehObjSetInPlacePool = BehaviorPool<BehObj_SetInPlace, g_wPoolCountBehObjSetInPlace>;

// VTABLE: HYPERX 0x004b249c
using BehObjSetMovingInPlacePool = BehaviorPool<BehObj_SetMovingInPlace, g_nPoolCountBehObjSetMovingInPlace>;

// VTABLE: HYPERX 0x004b2460
using BehObjTrackingPool = BehaviorPool<BehObj_Tracking, g_wPoolCountBehObjTracking>;

// VTABLE: HYPERX 0x004b2410
using BehObjTrackPassPrepPool = BehaviorPool<BehObj_TrackPassPrep, g_wPoolCountBehObjTrackPassPrep>;

// VTABLE: HYPERX 0x004b2424
using BehObjTrackTrajectoryPool = BehaviorPool<BehObj_TrackTrajectory, g_nPoolCountBehObjTrackTrajectory>;

// VTABLE: HYPERX 0x004b2578
using BehObjUpdateRelativePosPool = BehaviorPool<BehObj_UpdateRelativePos, g_nPoolCountBehObjUpdateRelativePos>;

// VTABLE: HYPERX 0x004b24d8
using BehObjUpdateWhileAttachedPool = BehaviorPool<BehObj_UpdateWhileAttached, g_nPoolCountBehObjUpdateWhileAttached>;

// VTABLE: HYPERX 0x004b2438
using BehObjWatchingPool = BehaviorPool<BehObj_Watching, g_wPoolCountBehObjWatching>;

#endif
