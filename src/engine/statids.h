// Ratings at 0xd20 are RatingMeasureID and match counters at 0xf24 StatID; the two differ.

#ifndef HYPERBLADE_STATIDS_H
#define HYPERBLADE_STATIDS_H

// A player's ratings. ObjectStats keeps rating n at m_aStats[n + 1], which RATING_SLOT gives.
constexpr auto RATING_WEIGHT_CLASS                = 0x00;
constexpr auto RATING_STAMINA                     = 0x01;
constexpr auto RATING_CHECKING                    = 0x02;
constexpr auto RATING_BALANCE                     = 0x03;
constexpr auto RATING_SPEED                       = 0x04;
constexpr auto RATING_SHOT_POWER                  = 0x05;
constexpr auto RATING_SHOT_ACCURACY               = 0x06;
constexpr auto RATING_OFFENSIVE_ABILITY           = 0x08;
constexpr auto RATING_LONG_SHOT_ENABLE            = 0x09;
constexpr auto RATING_IMMUNITY                    = 0x0a;
constexpr auto RATING_LONG_PASS_ENABLE            = 0x0b;
constexpr auto RATING_MULTI_POINT_SPREAD          = 0x0c;
constexpr auto RATING_ROK_HANDLING                = 0x0d;
constexpr auto RATING_ARMORY_RANGE                = 0x0e;
constexpr auto RATING_DAMAGE                      = 0x0f;
constexpr auto RATING_GENDER                      = 0x10;
constexpr auto RATING_SPEEDBURST0A                = 0x11;
constexpr auto RATING_PASS0                       = 0x1e;
constexpr auto RATING_PASS1                       = 0x1f;
constexpr auto RATING_PASS2                       = 0x20;
constexpr auto RATING_WIN_CLOSE_FREE_ROK          = 0x21;
constexpr auto RATING_LOSE_CLOSE_FREE_ROK         = 0x22;
constexpr auto RATING_WIN_FAR_FREE_ROK            = 0x23;
constexpr auto RATING_LOSE_FAR_FREE_ROK           = 0x24;
constexpr auto RATING_WIN_OFF_WING_JOB            = 0x25;
constexpr auto RATING_LOSE_OFF_WING_JOB           = 0x26;
constexpr auto RATING_MULTIPLIER0                 = 0x27;
constexpr auto RATING_MULTIPLIER1                 = 0x28;
constexpr auto RATING_TURNSTILE0                  = 0x29;
constexpr auto RATING_FIGHTING0                   = 0x2f;
constexpr auto RATING_HALFPIPE_REPERTOIRE         = 0x35;
constexpr auto RATING_CARRY_ROK_THROW_DIST        = 0x36;
constexpr auto RATING_WIN_CARRY_ROK_REGROUP_DIST  = 0x37;
constexpr auto RATING_LOSE_CARRY_ROK_REGROUP_DIST = 0x38;
constexpr auto RATING_WIN_AGGRESSION              = 0x39;
constexpr auto RATING_LOSE_AGGRESSION             = 0x3a;
constexpr auto RATING_SUB_OUT_DAMAGE              = 0x3b;
constexpr auto RATING_SUB_IN_DAMAGE               = 0x3c;
constexpr auto RATING_MIN_SUB_IN                  = 0x3e;
constexpr auto RATING_COUNT                       = 0x3f;

// A goalie has the first sixteen ratings only.
constexpr auto GOALIE_RATING_COUNT = 16;

#define RATING_SLOT(nRating) ((nRating) + 1)

// The match counters, id_STA_.
constexpr auto MATCH_STAT_GOALS                  = 0x00;
constexpr auto MATCH_STAT_SHOTS                  = 0x01;
constexpr auto MATCH_STAT_ASSISTS                = 0x02;
constexpr auto MATCH_STAT_SHOOTING               = 0x03;
constexpr auto MATCH_STAT_PASSING                = 0x04;
constexpr auto MATCH_STAT_PASSES_ATTEMPTED       = 0x05;
constexpr auto MATCH_STAT_PASSES_COMPLETED       = 0x06;
constexpr auto MATCH_STAT_TIME_OF_POSSESSION     = 0x07;
constexpr auto MATCH_STAT_TOP_STAMP_START        = 0x08;
constexpr auto MATCH_STAT_TOP_STAMP_END          = 0x09;
constexpr auto MATCH_STAT_2_POINT_SCORES         = 0x0a;
constexpr auto MATCH_STAT_2_POINT_COMPLETED      = 0x0c;
constexpr auto MATCH_STAT_SHORT_HANDED_ATTEMPTS  = 0x0e;
constexpr auto MATCH_STAT_SHORT_HANDED_COMPLETED = 0x0f;
constexpr auto MATCH_STAT_POWER_PLAY_ATTEMPTS    = 0x11;
constexpr auto MATCH_STAT_POWER_PLAY_COMPLETED   = 0x12;
constexpr auto MATCH_STAT_TIME_ON_RAIL           = 0x17;
constexpr auto MATCH_STAT_TOR_STAMP_START        = 0x18;
constexpr auto MATCH_STAT_TOR_STAMP_END          = 0x19;
constexpr auto MATCH_STAT_HANG_TIME              = 0x1b;
constexpr auto MATCH_STAT_HT_STAMP_START         = 0x1c;
constexpr auto MATCH_STAT_HT_STAMP_END           = 0x1d;
constexpr auto MATCH_STAT_ONE_TIMERS             = 0x1e;
constexpr auto MATCH_STAT_POWER_PLAY_TIME        = 0x20;
constexpr auto MATCH_STAT_PPT_STAMP_START        = 0x21;
constexpr auto MATCH_STAT_PPT_STAMP_END          = 0x22;
constexpr auto MATCH_STAT_1X_GOAL                = 0x23;
constexpr auto MATCH_STAT_TOTAL_GOALS            = 0x24;
constexpr auto MATCH_STAT_POSS_START             = 0x25;
constexpr auto MATCH_STAT_POSS_STOP              = 0x26;
constexpr auto MATCH_STAT_TOTAL_POINTS           = 0x27;
constexpr auto MATCH_STAT_SHOTS_ON_GOAL          = 0x28;
constexpr auto MATCH_STAT_SHOOTING_PCT           = 0x29;
constexpr auto MATCH_STAT_2X_GOAL                = 0x2a;
constexpr auto MATCH_STAT_4X_GOAL                = 0x2b;
constexpr auto MATCH_STAT_TIME_OF_POSS           = 0x2c;
constexpr auto MATCH_STAT_RANDOMIZERS            = 0x2d;
constexpr auto MATCH_STAT_KILLBALLS              = 0x2e;
constexpr auto MATCH_STAT_INJURIES               = 0x2f;
constexpr auto MATCH_STAT_FATALITIES             = 0x30;
constexpr auto MATCH_STAT_COUNT                  = 0x31;

#endif
