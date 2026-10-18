#ifndef HYPERBLADE_MESSAGEIDS_H
#define HYPERBLADE_MESSAGEIDS_H

// Scene message ids, numbered and named as the game's DATA/ENUMIDS.H numbers its id_MSG_ list.
constexpr auto MSG_SHOOT                = 0x00;
constexpr auto MSG_LOST_IT              = 0x02;
constexpr auto MSG_GOT_IT               = 0x03;
constexpr auto MSG_GOAL_SCORED          = 0x07;
constexpr auto MSG_JUMP                 = 0x08;
constexpr auto MSG_FLY                  = 0x09;
constexpr auto MSG_LANDED               = 0x0a;
constexpr auto MSG_ATTACH               = 0x0b;
constexpr auto MSG_HIT                  = 0x0c;
constexpr auto MSG_YER_ATTACKED         = 0x0f;
constexpr auto MSG_YER_RELEASED         = 0x10;
constexpr auto MSG_YER_HIT              = 0x11;
constexpr auto MSG_PASS_RECEIVED        = 0x15;
constexpr auto MSG_DEKE                 = 0x16;
constexpr auto MSG_SPUN_DOWN            = 0x17;
constexpr auto MSG_ATTACK               = 0x19;
constexpr auto MSG_SET_ANIMATION        = 0x1c;
constexpr auto MSG_MOVE_EYE_TRACK       = 0x1e;
constexpr auto MSG_THROW_WIND_UP        = 0x22;
constexpr auto MSG_CELL_ANIMATION_DONE  = 0x28;
constexpr auto MSG_GOT_THERE            = 0x2c;
constexpr auto MSG_KILL_BALL_CHANGE     = 0x2f;
constexpr auto MSG_END_PASS_MODE        = 0x35;
constexpr auto MSG_BLOCKED              = 0x36;
constexpr auto MSG_BREAKAWAY            = 0x37;
constexpr auto MSG_EXIT_BEHAVIOR        = 0x39;
constexpr auto MSG_BOUNCE               = 0x3a;
constexpr auto MSG_DEPLOY_WEAPON        = 0x3b;
constexpr auto MSG_NO_INPUT_FOR_SEC     = 0x40;
constexpr auto MSG_NEW_INPUT            = 0x43;
constexpr auto MSG_GOAL_RECEIVED        = 0x44;
constexpr auto MSG_TIME_OUT             = 0x45;
constexpr auto MSG_SPEED_BURST          = 0x46;
constexpr auto MSG_DRAG_OVER            = 0x48;
constexpr auto MSG_DOUBLE_SCORE         = 0x49;
constexpr auto MSG_INITIATE             = 0x4c;
constexpr auto MSG_TEST                 = 0x4d;
constexpr auto MSG_HEAD_CAM_OFF         = 0x4f;
constexpr auto MSG_SHOT_ON_GOAL         = 0x50;
constexpr auto MSG_PLAY_CD              = 0x54;
constexpr auto MSG_SUBSTITUTE_DROMER    = 0x57;
constexpr auto MSG_START_DRAG           = 0x5b;
constexpr auto MSG_GLIDE_TO_STOP        = 0x5e;
constexpr auto MSG_SYNC_FRAME           = 0x61;
constexpr auto MSG_WATCH_OUT_MULTIPLIER = 0x66;
constexpr auto MSG_WATCH_OUT_ARMORY     = 0x6e;
constexpr auto MSG_REALLY_SET_ANIMATION = 0x70;
constexpr auto MSG_TEAMMATE_COLLISION   = 0x75;
constexpr auto MSG_OPPONENT_COLLISION   = 0x76;
constexpr auto MSG_END_OF_PERIOD        = 0x87;
constexpr auto MSG_END_OF_GAME          = 0x88;
constexpr auto MSG_SYNC_POSSESSION      = 0x8b;
constexpr auto MSG_SYNC_SYSTEM          = 0x8c;
constexpr auto MSG_SYNC_DAMAGE          = 0x8d;
constexpr auto MSG_NOMSG                = 0x8e;

#endif
