#ifndef HYPERBLADE_AGENDAITEMPOOLS_H
#define HYPERBLADE_AGENDAITEMPOOLS_H

#include "common/nodepool.h"
#include "common/poolcounts.h"
#include "planvm/agendaitem.h"
#include "engine/cadjustacceleration.h"
#include "engine/cbeattached.h"
#include "engine/ccheckai.h"
#include "engine/ccircletrack.h"
#include "engine/cdisplaypassbar.h"
#include "engine/cdisplayshotpower.h"
#include "engine/cexplosionanimation.h"
#include "engine/cheadtrack.h"
#include "engine/cmonitorheldkeys.h"
#include "engine/cpansound.h"
#include "engine/crelaxyaw.h"
#include "engine/csetattitude.h"
#include "engine/csetpositionrelative.h"
#include "engine/cspeedburst.h"
#include "engine/csteertoposition.h"
#include "engine/ctrack.h"
#include "engine/ctrackpasserreceiver.h"
#include "engine/cwatch.h"
#include "planvm/cavoidobjects.h"
#include "planvm/cbungeetoward.h"
#include "planvm/cdopplersnd.h"
#include "planvm/cglideonsurface.h"
#include "planvm/cmonitorkeys.h"
#include "planvm/cpaletteeffect.h"
#include "planvm/cprojectandbounce.h"
#include "planvm/crelaxtoposition.h"
#include "planvm/cslideonrail.h"
#include "planvm/csteerbetweenfacing.h"
#include "planvm/csteertotarget.h"
#include "planvm/csteertoward.h"
#include "planvm/cstepanimation.h"
#include "planvm/ctimeractivity.h"
#include "planvm/ctracktraj.h"
#include "planvm/cupdateshotpower.h"
#include "planvm/cviewit.h"

template <typename T, auto& Count>
using AgendaItemPool = NodePool<AgendaItem, &AgendaItem::m_pNext, T, Count>;

// VTABLE: HYPERX 0x004b19b8
using CAdjustAccelerationPool = AgendaItemPool<CAdjustAcceleration, g_nPoolCountCAdjustAcceleration>;

// VTABLE: HYPERX 0x004b19a4
using CAvoidObjectsPool = AgendaItemPool<CAvoidObjects, g_wPoolCountCAvoidObjects>;

// VTABLE: HYPERX 0x004b1a30
using CBeAttachedPool = AgendaItemPool<CBeAttached, g_nPoolCountCBeAttached>;

// VTABLE: HYPERX 0x004b1990
using CBungeeTowardPool = AgendaItemPool<CBungeeToward, g_nPoolCountCBungeeToward>;

// VTABLE: HYPERX 0x004b17d8
using CCheckAIPool = AgendaItemPool<CCheckAI, g_nPoolCountCCheckAI>;

// VTABLE: HYPERX 0x004b17c4
using CCircleTrackPool = AgendaItemPool<CCircleTrack, g_wPoolCountCCircleTrack>;

// VTABLE: HYPERX 0x004b1800
using CDisplayPassBarPool = AgendaItemPool<CDisplayPassBar, g_wPoolCountCDisplayPassBar>;

// VTABLE: HYPERX 0x004b1828
using CDisplayShotPowerPool = AgendaItemPool<CDisplayShotPower, g_wPoolCountCDisplayShotPower>;

// VTABLE: HYPERX 0x004b1864
using CDopplerSndPool = AgendaItemPool<CDopplerSnd, g_nPoolCountCDopplerSnd>;

// VTABLE: HYPERX 0x004b17ec
using CExplosionAnimationPool = AgendaItemPool<CExplosionAnimation, g_wPoolCountCExplosionAnimation>;

// VTABLE: HYPERX 0x004b1a6c
using CGlideOnSurfacePool = AgendaItemPool<CGlideOnSurface, g_nPoolCountCGlideOnSurface>;

// VTABLE: HYPERX 0x004b1850
using CHeadTrackPool = AgendaItemPool<CHeadTrack, g_wPoolCountCHeadTrack>;

// VTABLE: HYPERX 0x004b192c
using CMonitorHeldKeysPool = AgendaItemPool<CMonitorHeldKeys, g_wPoolCountCMonitorHeldKeys>;

// VTABLE: HYPERX 0x004b1940
using CMonitorKeysPool = AgendaItemPool<CMonitorKeys, g_nPoolCountCMonitorKeys>;

// VTABLE: HYPERX 0x004b183c
using CPaletteEffectPool = AgendaItemPool<CPaletteEffect, g_wPoolCountCPaletteEffect>;

// VTABLE: HYPERX 0x004b1878
using CPanSoundPool = AgendaItemPool<CPanSound, g_nPoolCountCPanSound>;

// VTABLE: HYPERX 0x004b1a44
using CProjectAndBouncePool = AgendaItemPool<CProjectAndBounce, g_wPoolCountCProjectAndBounce>;

// VTABLE: HYPERX 0x004b1904
using CRelaxToPositionPool = AgendaItemPool<CRelaxToPosition, g_nPoolCountCRelaxToPosition>;

// VTABLE: HYPERX 0x004b1968
using CRelaxYawPool = AgendaItemPool<CRelaxYaw, g_nPoolCountCRelaxYaw>;

// VTABLE: HYPERX 0x004b197c
using CSetAttitudePool = AgendaItemPool<CSetAttitude, g_wPoolCountCSetAttitude>;

// VTABLE: HYPERX 0x004b1a08
using CSetPositionRelativePool = AgendaItemPool<CSetPositionRelative, g_nPoolCountCSetPositionRelative>;

// VTABLE: HYPERX 0x004b1a58
using CSlideOnRailPool = AgendaItemPool<CSlideOnRail, g_nPoolCountCSlideOnRail>;

// VTABLE: HYPERX 0x004b19e0
using CSteerBetweenFacingPool = AgendaItemPool<CSteerBetweenFacing, g_nPoolCountCSteerBetweenFacing>;

// VTABLE: HYPERX 0x004b19f4
using CSteerToPositionPool = AgendaItemPool<CSteerToPosition, g_wPoolCountCSteerToPosition>;

// VTABLE: HYPERX 0x004b1a1c
using CSteerToTargetPool = AgendaItemPool<CSteerToTarget, g_wPoolCountCSteerToTarget>;

// VTABLE: HYPERX 0x004b19cc
using CSteerTowardPool = AgendaItemPool<CSteerToward, g_wPoolCountCSteerToward>;

// VTABLE: HYPERX 0x004b1954
using CStepAnimationPool = AgendaItemPool<CStepAnimation, g_nPoolCountCStepAnimation>;

// VTABLE: HYPERX 0x004b18f0
using CTimerActivityPool = AgendaItemPool<CTimerActivity, g_wPoolCountCTimerActivity>;

// VTABLE: HYPERX 0x004b18a0
using CTrackPasser_ReceiverPool = AgendaItemPool<CTrackPasser_Receiver, g_wPoolCountCTrackPasser_Receiver>;

// VTABLE: HYPERX 0x004b18dc
using CTrackPool = AgendaItemPool<CTrack, g_wPoolCountCTrack>;

// VTABLE: HYPERX 0x004b18c8
using CTrackTrajPool = AgendaItemPool<CTrackTraj, g_wPoolCountCTrackTraj>;

// VTABLE: HYPERX 0x004b1814
using CUpdateShotPowerPool = AgendaItemPool<CUpdateShotPower, g_nPoolCountCUpdateShotPower>;

// VTABLE: HYPERX 0x004b188c
using CViewItPool = AgendaItemPool<CViewIt, g_wPoolCountCViewIt>;

// VTABLE: HYPERX 0x004b18b4
using CWatchPool = AgendaItemPool<CWatch, g_wPoolCountCWatch>;

// VTABLE: HYPERX 0x004b1918
using CSpeedBurstPool = AgendaItemPool<CSpeedBurst, g_nPoolCountCSpeedBurst>;

#endif
