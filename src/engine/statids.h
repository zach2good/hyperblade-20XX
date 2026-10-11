// Ratings at 0xd20 are RatingMeasureID and match counters at 0xf24 StatID; the two differ.

#ifndef HYPERBLADE_STATIDS_H
#define HYPERBLADE_STATIDS_H

// A player's ratings. ObjectStats keeps rating n at m_aStats[n + 1], which RATING_SLOT gives.
#define RATING_WEIGHT_CLASS                0x00
#define RATING_STAMINA                     0x01
#define RATING_CHECKING                    0x02
#define RATING_BALANCE                     0x03
#define RATING_SPEED                       0x04
#define RATING_SHOT_POWER                  0x05
#define RATING_SHOT_ACCURACY               0x06
#define RATING_OFFENSIVE_ABILITY           0x08
#define RATING_LONG_SHOT_ENABLE            0x09
#define RATING_IMMUNITY                    0x0a
#define RATING_LONG_PASS_ENABLE            0x0b
#define RATING_MULTI_POINT_SPREAD          0x0c
#define RATING_ROK_HANDLING                0x0d
#define RATING_ARMORY_RANGE                0x0e
#define RATING_DAMAGE                      0x0f
#define RATING_GENDER                      0x10
#define RATING_SPEEDBURST0A                0x11
#define RATING_PASS0                       0x1e
#define RATING_PASS1                       0x1f
#define RATING_PASS2                       0x20
#define RATING_WIN_CLOSE_FREE_ROK          0x21
#define RATING_LOSE_CLOSE_FREE_ROK         0x22
#define RATING_WIN_FAR_FREE_ROK            0x23
#define RATING_LOSE_FAR_FREE_ROK           0x24
#define RATING_WIN_OFF_WING_JOB            0x25
#define RATING_LOSE_OFF_WING_JOB           0x26
#define RATING_MULTIPLIER0                 0x27
#define RATING_MULTIPLIER1                 0x28
#define RATING_TURNSTILE0                  0x29
#define RATING_FIGHTING0                   0x2f
#define RATING_HALFPIPE_REPERTOIRE         0x35
#define RATING_CARRY_ROK_THROW_DIST        0x36
#define RATING_WIN_CARRY_ROK_REGROUP_DIST  0x37
#define RATING_LOSE_CARRY_ROK_REGROUP_DIST 0x38
#define RATING_WIN_AGGRESSION              0x39
#define RATING_LOSE_AGGRESSION             0x3a
#define RATING_SUB_OUT_DAMAGE              0x3b
#define RATING_SUB_IN_DAMAGE               0x3c
#define RATING_MIN_SUB_IN                  0x3e
#define RATING_COUNT                       0x3f

// A goalie has the first sixteen ratings only.
#define GOALIE_RATING_COUNT 16

#define RATING_SLOT(nRating) ((nRating) + 1)

// The match counters, id_STA_.
#define MATCH_STAT_GOALS                  0x00
#define MATCH_STAT_SHOTS                  0x01
#define MATCH_STAT_ASSISTS                0x02
#define MATCH_STAT_SHOOTING               0x03
#define MATCH_STAT_PASSING                0x04
#define MATCH_STAT_PASSES_ATTEMPTED       0x05
#define MATCH_STAT_PASSES_COMPLETED       0x06
#define MATCH_STAT_TIME_OF_POSSESSION     0x07
#define MATCH_STAT_TOP_STAMP_START        0x08
#define MATCH_STAT_TOP_STAMP_END          0x09
#define MATCH_STAT_2_POINT_SCORES         0x0a
#define MATCH_STAT_2_POINT_COMPLETED      0x0c
#define MATCH_STAT_SHORT_HANDED_ATTEMPTS  0x0e
#define MATCH_STAT_SHORT_HANDED_COMPLETED 0x0f
#define MATCH_STAT_POWER_PLAY_ATTEMPTS    0x11
#define MATCH_STAT_POWER_PLAY_COMPLETED   0x12
#define MATCH_STAT_TIME_ON_RAIL           0x17
#define MATCH_STAT_TOR_STAMP_START        0x18
#define MATCH_STAT_TOR_STAMP_END          0x19
#define MATCH_STAT_HANG_TIME              0x1b
#define MATCH_STAT_HT_STAMP_START         0x1c
#define MATCH_STAT_HT_STAMP_END           0x1d
#define MATCH_STAT_ONE_TIMERS             0x1e
#define MATCH_STAT_POWER_PLAY_TIME        0x20
#define MATCH_STAT_PPT_STAMP_START        0x21
#define MATCH_STAT_PPT_STAMP_END          0x22
#define MATCH_STAT_1X_GOAL                0x23
#define MATCH_STAT_TOTAL_GOALS            0x24
#define MATCH_STAT_POSS_START             0x25
#define MATCH_STAT_POSS_STOP              0x26
#define MATCH_STAT_TOTAL_POINTS           0x27
#define MATCH_STAT_SHOTS_ON_GOAL          0x28
#define MATCH_STAT_SHOOTING_PCT           0x29
#define MATCH_STAT_2X_GOAL                0x2a
#define MATCH_STAT_4X_GOAL                0x2b
#define MATCH_STAT_TIME_OF_POSS           0x2c
#define MATCH_STAT_RANDOMIZERS            0x2d
#define MATCH_STAT_KILLBALLS              0x2e
#define MATCH_STAT_INJURIES               0x2f
#define MATCH_STAT_FATALITIES             0x30
#define MATCH_STAT_COUNT                  0x31

#endif
