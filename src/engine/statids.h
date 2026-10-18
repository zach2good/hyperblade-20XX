// Ratings at 0xd20 are RatingMeasureID and match counters at 0xf24 StatID; the two differ.

#ifndef HYPERBLADE_STATIDS_H
#define HYPERBLADE_STATIDS_H

// A player's ratings. ObjectStats keeps rating n at m_aStats[n + 1], which RATING_SLOT gives.
constexpr auto kRatingWeightClass             = 0x00;
constexpr auto kRatingStamina                 = 0x01;
constexpr auto kRatingChecking                = 0x02;
constexpr auto kRatingBalance                 = 0x03;
constexpr auto kRatingSpeed                   = 0x04;
constexpr auto kRatingShotPower               = 0x05;
constexpr auto kRatingShotAccuracy            = 0x06;
constexpr auto kRatingOffensiveAbility        = 0x08;
constexpr auto kRatingLongShotEnable          = 0x09;
constexpr auto kRatingImmunity                = 0x0a;
constexpr auto kRatingLongPassEnable          = 0x0b;
constexpr auto kRatingMultiPointSpread        = 0x0c;
constexpr auto kRatingRokHandling             = 0x0d;
constexpr auto kRatingArmoryRange             = 0x0e;
constexpr auto kRatingDamage                  = 0x0f;
constexpr auto kRatingGender                  = 0x10;
constexpr auto kRatingSpeedBurst0a            = 0x11;
constexpr auto kRatingPass0                   = 0x1e;
constexpr auto kRatingPass1                   = 0x1f;
constexpr auto kRatingPass2                   = 0x20;
constexpr auto kRatingWinCloseFreeRok         = 0x21;
constexpr auto kRatingLoseCloseFreeRok        = 0x22;
constexpr auto kRatingWinFarFreeRok           = 0x23;
constexpr auto kRatingLoseFarFreeRok          = 0x24;
constexpr auto kRatingWinOffWingJob           = 0x25;
constexpr auto kRatingLoseOffWingJob          = 0x26;
constexpr auto kRatingMultiplier0             = 0x27;
constexpr auto kRatingMultiplier1             = 0x28;
constexpr auto kRatingTurnstile0              = 0x29;
constexpr auto kRatingFighting0               = 0x2f;
constexpr auto kRatingHalfPipeRepertoire      = 0x35;
constexpr auto kRatingCarryRokThrowDist       = 0x36;
constexpr auto kRatingWinCarryRokRegroupDist  = 0x37;
constexpr auto kRatingLoseCarryRokRegroupDist = 0x38;
constexpr auto kRatingWinAggression           = 0x39;
constexpr auto kRatingLoseAggression          = 0x3a;
constexpr auto kRatingSubOutDamage            = 0x3b;
constexpr auto kRatingSubInDamage             = 0x3c;
constexpr auto kRatingMinSubIn                = 0x3e;
constexpr auto kRatingCount                   = 0x3f;

// A goalie has the first sixteen ratings only.
constexpr auto kGoalieRatingCount = 16;

#define RATING_SLOT(nRating) ((nRating) + 1)

// The match counters, id_STA_.
constexpr auto kMatchStatGoals                = 0x00;
constexpr auto kMatchStatShots                = 0x01;
constexpr auto kMatchStatAssists              = 0x02;
constexpr auto kMatchStatShooting             = 0x03;
constexpr auto kMatchStatPassing              = 0x04;
constexpr auto kMatchStatPassesAttempted      = 0x05;
constexpr auto kMatchStatPassesCompleted      = 0x06;
constexpr auto kMatchStatTimeOfPossession     = 0x07;
constexpr auto kMatchStatTopStampStart        = 0x08;
constexpr auto kMatchStatTopStampEnd          = 0x09;
constexpr auto kMatchStat2PointScores         = 0x0a;
constexpr auto kMatchStat2PointCompleted      = 0x0c;
constexpr auto kMatchStatShortHandedAttempts  = 0x0e;
constexpr auto kMatchStatShortHandedCompleted = 0x0f;
constexpr auto kMatchStatPowerPlayAttempts    = 0x11;
constexpr auto kMatchStatPowerPlayCompleted   = 0x12;
constexpr auto kMatchStatTimeOnRail           = 0x17;
constexpr auto kMatchStatTorStampStart        = 0x18;
constexpr auto kMatchStatTorStampEnd          = 0x19;
constexpr auto kMatchStatHangTime             = 0x1b;
constexpr auto kMatchStatHtStampStart         = 0x1c;
constexpr auto kMatchStatHtStampEnd           = 0x1d;
constexpr auto kMatchStatOneTimers            = 0x1e;
constexpr auto kMatchStatPowerPlayTime        = 0x20;
constexpr auto kMatchStatPptStampStart        = 0x21;
constexpr auto kMatchStatPptStampEnd          = 0x22;
constexpr auto kMatchStat1xGoal               = 0x23;
constexpr auto kMatchStatTotalGoals           = 0x24;
constexpr auto kMatchStatPossStart            = 0x25;
constexpr auto kMatchStatPossStop             = 0x26;
constexpr auto kMatchStatTotalPoints          = 0x27;
constexpr auto kMatchStatShotsOnGoal          = 0x28;
constexpr auto kMatchStatShootingPct          = 0x29;
constexpr auto kMatchStat2xGoal               = 0x2a;
constexpr auto kMatchStat4xGoal               = 0x2b;
constexpr auto kMatchStatTimeOfPoss           = 0x2c;
constexpr auto kMatchStatRandomizers          = 0x2d;
constexpr auto kMatchStatKillballs            = 0x2e;
constexpr auto kMatchStatInjuries             = 0x2f;
constexpr auto kMatchStatFatalities           = 0x30;
constexpr auto kMatchStatCount                = 0x31;

#endif
