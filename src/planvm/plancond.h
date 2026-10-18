#ifndef HYPERBLADE_PLANCOND_H
#define HYPERBLADE_PLANCOND_H

#include "common/compat.h"

class Interactor;
class EngineObject;
class AgendaItem;
struct PlanNode;

// DATA/ENUMIDS.H's ID_ATK: the attack a YerAttacked message carries.
enum AttackType
{
    kAttackPunch,
    kAttackHipCheck,
    kAttackClothesLineL,
    kAttackClothesLineR,
    kAttackShoulderCheckL,
    kAttackShoulderCheckR,
    kAttackLegSweepL,
    kAttackLegSweepR
};

namespace PlanCond
{

// The linker folded these empty bodies onto Stub::Assert, which carries the marker.
inline void AnalyzePadC(Interactor*, PlanNode*, short*, int)
{
}

inline void TryForKillball(Interactor*, PlanNode*, short*, int)
{
}

inline void TargetAcquired(Interactor*, PlanNode*, short*, int)
{
}

void DecideGoalieLaunch(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void Decide_Attachment(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ForceRearView(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void OkToLaunchRok(Interactor* pContext, PlanNode* pArgs, short* pOutValid, int nArg);
void SetInteractorName(EngineObject* pActor, PlanNode* pNode, class GameBehaviorObject* pBehaviour, Interactor* pInteractor);
void SetActiveState(EngineObject* pActor, PlanNode* pNode, class GameBehaviorObject* pBehaviour, Interactor* pInteractor);
void Decide_Celebration(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void EngageWhichOpponent(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void TeammateHasRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void GagTheAnnouncer(Interactor* pContext, PlanNode* pArgs, short* pnOut, int nArg);
void VictimCamIsOn(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void IsScanningModeOn(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AttackerIsInRear(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AnalyzePadA(Interactor* pContext, PlanNode* pArgs, short* pResult, int nSelector);
void AnalyzePadB(Interactor* pContext, PlanNode* pArgs, short* pResult, int nKind);
void InRangeToAttack(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void Decide_BlockOrGoOut(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SpeedBurstIsRelaxed(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void WhichOppnIsClosestToRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void BallGotBehindMe(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void GoaliePassingTarget(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void WhichOppnToAttack(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void WhichOppnIsClosestToMe(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void ClosestTargetToProjectile(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ClosestUncoveredOppn(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void NearestSuspenseMult(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void OffenseOrWRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void HostIsViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RokInWhichTeamsPoss(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void WhoHasRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void IsDoubleScore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void BungeeVictimLorR(Interactor* pContext, PlanNode* pArgs, short* pResult, int nFlags);
void Decide_PassOrRun(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void Decide_HPSpin_Rail_orNot(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void PickACell(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void Random(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ShotPower(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void IfInFaceOff(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void TeamIsDead(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void TeamIsInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void MyTeamIsInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void WhoWonTheGame(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void IsSenderTeammate(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AmMainDromer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void MyID(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);

// A message's data word packs one field in each nibble.
inline short MessageNibble(unsigned int nData, int nShift)
{
    return static_cast<short>((nData >> nShift) & 0xf);
}

void MessageDataNibl0(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void MessageDataNibl1(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void MessageDataNibl4(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void MsgData4Nibl1X2Nibl0(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SideKeysAreDown(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AmViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void TeammateIsViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void TeammateIsHumanOrInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void OkToSwitchToTeammate(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void OkToPassToTeammate(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void DecidePosition(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void CollisionDetail(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void OwnerIsViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ViewOwnersMateInFront(Interactor* pContext, PlanNode* pArgs, short* pOutFacing, int nArg);
void IHaveRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nFlag);
void DromerHasRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void SetResultDirect(Interactor* pContext, PlanNode* pArgs, short* pResult, int nValue);
void IsRokInMyZone(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SubstitutionSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void PickValidSubstitute(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void OkToSubstituteTeammate(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void GetImmunity(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AttachedToSender(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AmIKnockedLoose(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RokScoreValue(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void CatchBlockOrMiss(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void Jumping(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RandomInactiveWeapon(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RandomEmptyArmory(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void WhoKilledMe(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void TakeDamage(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAttack);
void AttackedAnim(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAttack);
void SpecialAttacks(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAnim);
void MeOrTeammateClosestToRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void TenOrMorePtSpread(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RedIsAvailable(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RatingMaxedOut(Interactor* pContext, PlanNode* pNode, short* pnResult, int nArg);
void ChooseTeamTune(Interactor* pContext, PlanNode* pArgs, short* pResult, int nTeam);
void MyTeamLosingByAtLeast(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void DecideWithRating(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AttackOrDeployWeapon(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void DromerNearMine(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RokValueInMyFavor(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void CheckPassingDistance(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RecvrInRangeNView(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ChoosePassingPlay(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void Ducking(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RokIsCharged(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RokIsChargedAgainst(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SpinUp(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void PassOrRunByRatings(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void LooseRokWhatToDo(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void JostledDropRok(Interactor* pContext, PlanNode* pNode, short* pnResult, int nArg);
void GoThroughMuliplier(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void PickAttackMove(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void OffWingJob(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void PickFlyingMove(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void PickHalfpipeMove(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AvoidSpinningTurnstile(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void DecideNever(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ShootSituation(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RegroupSituation(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void AggressionOnOthers(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void NearObjectBy(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void OkToGetRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void CheckScoreOnOwnGoal(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void Interception(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void RokInRange(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void ClosestDromerToRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void SoundBank(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void GetTeamParity(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void WinningOrLosing(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void IsDromerKnockedDown(Interactor* pContext, PlanNode* pArgs, short* pResult, int nDefault);
void IsDromerInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void CheckRenderFlag(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);

} // namespace PlanCond

#endif
