#ifndef HYPERBLADE_PLANACTION_H
#define HYPERBLADE_PLANACTION_H

#include "common/compat.h"

class Interactor;
#include "planvm/plannode.h"

namespace PlanAction
{

// The linker folded these empty bodies onto Stub::Assert, which carries the marker.
inline void NoteStats4Possessors(Interactor*, PlanNode*, short*, int)
{
}

inline void GetKeystateFlags(Interactor*, PlanNode*, short*, int)
{
}

inline void SetInputMode(Interactor*, PlanNode*, short*, int)
{
}

inline void SetLaserHurdleCharge(Interactor*, PlanNode*, short*, int)
{
}

inline void KillBallStaging(Interactor*, PlanNode*, short*, int)
{
}

inline void KillBallExecute(Interactor*, PlanNode*, short*, int)
{
}

void         SetTrackHeight(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetMinHeight(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetLookHeight(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetCamBufferFactor(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetScanModes(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetSolarWindFactor(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetRenderFlag(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl IncRokValue(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetRokKillState(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl GrayscaleRandomizer(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetLocalReal(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetInDrag(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void         SetOnRim(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void         SetFriction(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void         SetMyGravity(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl ModifyAvailableReds(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl ModifyRating(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl SwapRandomizers(Interactor* pContext, PlanNode* pNode, short* pState, int nPassed);
void __cdecl SetKilledDromer(Interactor* pContext, PlanNode* pArgs, short* pState, int nPrevSlot);
void         ResetCameraTrack(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         ClearBehaviorAnimation(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         FlyingSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         AnimationSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         ClockStoppedSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl DesiredSubstitute(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPassed);
void __cdecl StartStatTimer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void __cdecl EndStatTimer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void __cdecl DeployWeapon(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetImmunity(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetAnimation(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void __cdecl RestorePersistentAnimation(Interactor* pOpcode, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetAnimationEvent(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void __cdecl SetAnimationState(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetAnimationSpeed(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl IncrAnimationSpeed(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl SetAnimationDoneMessage(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void         Set_AttachPak(Interactor* pContext, PlanNode* pArgs, short* pResult, int nSlot);
void         Unset_AttachPak(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl SetObjectToTrack(Interactor* pContext, PlanNode* pArgs, short* pResult, int nSlot);
void         IfNotRokTrackMe(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         SetTrackToViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl SetTrackingOffset(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl IncrementTrackingOffset(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl IncrementCameraDir(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         IncrementBungeeLength(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetBungeeLength(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetOffsetValues(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetObjectToWatch(Interactor* pContext, PlanNode* pArgs, short* pState, int nSlot);
void         IncrementObjectToWatch(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         IncrementObjectToTrack(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         ResetCameraBufferSize(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl ResetReticle(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetHeadCamText(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl InitRearViewPos(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetCamOffsetToLocation(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         RelaxYawToFace(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl ExplodeFlat(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl ExplodeAgainstVelocity(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl ExplodeWithVelocity(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl ExplodeUniform(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl SmallBloodSpurt(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl LargeBloodSpurt(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl LoseHead(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl RegainHead(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl BodySplit(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl SendLaunchMessage(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl Launch(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl Throw(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl KickDownToSurface(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl LaunchAt(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl LaunchToward(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl RecoilOffWall(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void __cdecl ReflectOffWall(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl ReflectOffModel(Interactor* pContext, PlanNode* pNode, short* pState, int nSlot);
void __cdecl ClothesLineRecoil(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl ManOnManRecoil(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl BounceOffDromer(Interactor* pContext, PlanNode* pParams, short* pState, int nArg);
void __cdecl LaunchGunDirection(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetPaletteEffect(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void         SetDeviceOwner(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SignalKeystate(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl ReallySendMsg(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SendMsg(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void __cdecl SyncPossession(Interactor* pActor, PlanNode* pArgs, short* pState, int nArg);
void         SyncSystem(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         SyncDamage(Interactor* pContext, PlanNode* pArgs, short* pResult, int nValue);
void __cdecl TransferViewOwnership(Interactor* pContext, PlanNode* pArgs, short* pState, int nSlot);
void __cdecl SendMessageToMyCamera(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void __cdecl Set_SpeedBurst(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl SetAttackee(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetKnockedDownState(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         HoverAboveArmory(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl SetLocation(Interactor* pContext, PlanNode* pNode, short* pState, int nMarkerIndex);
void __cdecl SetVelocity(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl Engage_Opponent(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl IncrementStat(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void __cdecl ScaleBubToRating(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void         SetTargetWithData(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl BungeeTowardTarget(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl KickOffRim(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         EndPeriodNow(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SubstitutePlayer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void         TrackTurnstile(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void __cdecl CreditDamage(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         SetMaxSpeed(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl ModifyDamage(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void         IncrementScore(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl ModifyOpposTeamFriction(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void __cdecl ModifySpeedRating(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void __cdecl ThrowBungee(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl SpinTurnstile(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl ModifyPitch(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void __cdecl HookDragBungeesOn(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void         ResetAccTransient(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl ResetKeystateFlags(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void         IncrAnimSpeedWRating(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void __cdecl SpeedBurstSituation(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void         SetShotPowerMeter(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl GlideOnRamp(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void         Trace(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetInfoBarText(Interactor* pContext, PlanNode* pNode, short* pResult, int nParam);
void __cdecl SetInfoTextDromer(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl SetInfoTextTeam(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);

} // namespace PlanAction

#endif
