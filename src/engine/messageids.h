#ifndef HYPERBLADE_MESSAGEIDS_H
#define HYPERBLADE_MESSAGEIDS_H

// Scene message ids, numbered and named as the game's DATA/ENUMIDS.H numbers its id_MSG_ list.
constexpr auto kMsgShoot              = 0x00;
constexpr auto kMsgLostIt             = 0x02;
constexpr auto kMsgGotIt              = 0x03;
constexpr auto kMsgGoalScored         = 0x07;
constexpr auto kMsgJump               = 0x08;
constexpr auto kMsgFly                = 0x09;
constexpr auto kMsgLanded             = 0x0a;
constexpr auto kMsgAttach             = 0x0b;
constexpr auto kMsgHit                = 0x0c;
constexpr auto kMsgYerAttacked        = 0x0f;
constexpr auto kMsgYerReleased        = 0x10;
constexpr auto kMsgYerHit             = 0x11;
constexpr auto kMsgPassReceived       = 0x15;
constexpr auto kMsgDeke               = 0x16;
constexpr auto kMsgSpunDown           = 0x17;
constexpr auto kMsgAttack             = 0x19;
constexpr auto kMsgSetAnimation       = 0x1c;
constexpr auto kMsgMoveEyeTrack       = 0x1e;
constexpr auto kMsgThrowWindUp        = 0x22;
constexpr auto kMsgCellAnimationDone  = 0x28;
constexpr auto kMsgGotThere           = 0x2c;
constexpr auto kMsgKillBallChange     = 0x2f;
constexpr auto kMsgEndPassMode        = 0x35;
constexpr auto kMsgBlocked            = 0x36;
constexpr auto kMsgBreakaway          = 0x37;
constexpr auto kMsgExitBehavior       = 0x39;
constexpr auto kMsgBounce             = 0x3a;
constexpr auto kMsgDeployWeapon       = 0x3b;
constexpr auto kMsgNoInputForSec      = 0x40;
constexpr auto kMsgNewInput           = 0x43;
constexpr auto kMsgGoalReceived       = 0x44;
constexpr auto kMsgTimeOut            = 0x45;
constexpr auto kMsgSpeedBurst         = 0x46;
constexpr auto kMsgDragOver           = 0x48;
constexpr auto kMsgDoubleScore        = 0x49;
constexpr auto kMsgInitiate           = 0x4c;
constexpr auto kMsgTest               = 0x4d;
constexpr auto kMsgHeadCamOff         = 0x4f;
constexpr auto kMsgShotOnGoal         = 0x50;
constexpr auto kMsgPlayCD             = 0x54;
constexpr auto kMsgSubstituteDromer   = 0x57;
constexpr auto kMsgStartDrag          = 0x5b;
constexpr auto kMsgGlideToStop        = 0x5e;
constexpr auto kMsgSyncFrame          = 0x61;
constexpr auto kMsgWatchOutMultiplier = 0x66;
constexpr auto kMsgWatchOutArmory     = 0x6e;
constexpr auto kMsgReallySetAnimation = 0x70;
constexpr auto kMsgTeammateCollision  = 0x75;
constexpr auto kMsgOpponentCollision  = 0x76;
constexpr auto kMsgExtra1             = 0x7c;
constexpr auto kMsgExtra6             = 0x81;
constexpr auto kMsgExtra7             = 0x82;
constexpr auto kMsgEndOfPeriod        = 0x87;
constexpr auto kMsgEndOfGame          = 0x88;
constexpr auto kMsgSyncPossession     = 0x8b;
constexpr auto kMsgSyncSystem         = 0x8c;
constexpr auto kMsgSyncDamage         = 0x8d;
constexpr auto kMsgNomsg              = 0x8e;

#endif
