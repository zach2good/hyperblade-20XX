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

void SetTrackHeight(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetMinHeight(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetLookHeight(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetCamBufferFactor(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetScanModes(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetSolarWindFactor(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetRenderFlag(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void IncRokValue(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetRokKillState(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void GrayscaleRandomizer(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetLocalReal(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetInDrag(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void SetOnRim(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void SetFriction(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void SetMyGravity(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ModifyAvailableReds(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ModifyRating(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SwapRandomizers(Interactor* pContext, PlanNode* pNode, short* pState, int nPassed);
void SetKilledDromer(Interactor* pContext, PlanNode* pArgs, short* pState, int nPrevSlot);
void ResetCameraTrack(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ClearBehaviorAnimation(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void FlyingSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AnimationSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ClockStoppedSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void DesiredSubstitute(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPassed);
void StartStatTimer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void EndStatTimer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void DeployWeapon(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetImmunity(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetAnimation(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void RestorePersistentAnimation(Interactor* pOpcode, PlanNode* pArgs, short* pState, int nArg);
void SetAnimationEvent(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void SetAnimationState(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetAnimationSpeed(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void IncrAnimationSpeed(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void SetAnimationDoneMessage(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void Set_AttachPak(Interactor* pContext, PlanNode* pArgs, short* pResult, int nSlot);
void Unset_AttachPak(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SetObjectToTrack(Interactor* pContext, PlanNode* pArgs, short* pResult, int nSlot);
void IfNotRokTrackMe(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SetTrackToViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SetTrackingOffset(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void IncrementTrackingOffset(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void IncrementCameraDir(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void IncrementBungeeLength(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetBungeeLength(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetOffsetValues(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetObjectToWatch(Interactor* pContext, PlanNode* pArgs, short* pState, int nSlot);
void IncrementObjectToWatch(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void IncrementObjectToTrack(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ResetCameraBufferSize(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ResetReticle(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetHeadCamText(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void InitRearViewPos(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetCamOffsetToLocation(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void RelaxYawToFace(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ExplodeFlat(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void ExplodeAgainstVelocity(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void ExplodeWithVelocity(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void ExplodeUniform(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void SmallBloodSpurt(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void LargeBloodSpurt(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void LoseHead(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void RegainHead(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void BodySplit(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void SendLaunchMessage(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void Launch(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void Throw(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void KickDownToSurface(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void LaunchAt(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void LaunchToward(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void RecoilOffWall(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void ReflectOffWall(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void ReflectOffModel(Interactor* pContext, PlanNode* pNode, short* pState, int nSlot);
void ClothesLineRecoil(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ManOnManRecoil(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void BounceOffDromer(Interactor* pContext, PlanNode* pParams, short* pState, int nArg);
void LaunchGunDirection(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetPaletteEffect(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void SetDeviceOwner(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SignalKeystate(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ReallySendMsg(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SendMsg(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void SyncPossession(Interactor* pActor, PlanNode* pArgs, short* pState, int nArg);
void SyncSystem(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SyncDamage(Interactor* pContext, PlanNode* pArgs, short* pResult, int nValue);
void TransferViewOwnership(Interactor* pContext, PlanNode* pArgs, short* pState, int nSlot);
void SendMessageToMyCamera(Interactor* pContext, PlanNode* pArgs, short* pState, int nParam);
void Set_SpeedBurst(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void SetAttackee(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetKnockedDownState(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void HoverAboveArmory(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void SetLocation(Interactor* pContext, PlanNode* pNode, short* pState, int nMarkerIndex);
void SetVelocity(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void Engage_Opponent(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void IncrementStat(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void ScaleBubToRating(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void SetTargetWithData(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void BungeeTowardTarget(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void KickOffRim(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void EndPeriodNow(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SubstitutePlayer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void TrackTurnstile(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void CreditDamage(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetMaxSpeed(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ModifyDamage(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void IncrementScore(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void ModifyOpposTeamFriction(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void ModifySpeedRating(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void ThrowBungee(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void SpinTurnstile(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void ModifyPitch(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void HookDragBungeesOn(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void ResetAccTransient(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ResetKeystateFlags(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void IncrAnimSpeedWRating(Interactor* pContext, PlanNode* pNode, short* pResult, int nArg);
void SpeedBurstSituation(Interactor* pContext, PlanNode* pNode, short* pState, int nArg);
void SetShotPowerMeter(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void GlideOnRamp(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void Trace(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetInfoBarText(Interactor* pContext, PlanNode* pNode, short* pResult, int nParam);
void SetInfoTextDromer(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void SetInfoTextTeam(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);

} // namespace PlanAction

#endif
