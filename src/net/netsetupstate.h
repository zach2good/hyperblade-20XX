// Fourteen values under twenty-two names; files disagree and nothing in the image settles it.

#ifndef HYPERBLADE_NETSETUPSTATE_H
#define HYPERBLADE_NETSETUPSTATE_H

enum NetSetupState
{
    // Nothing chosen yet. Two files, two names.
    NETSETUP_STATE_START = 0,
    NETSETUP_STATE_IDLE  = 0,

    NETSETUP_STATE_PROVIDER = 1,

    // Looking at the session list. The second name is one file's.
    NETSETUP_STATE_BROWSING = 2,
    NETSETUP_STATE_JOINED   = 2,

    NETSETUP_STATE_SELECTED  = 3,
    NETSETUP_STATE_WAITING_B = 3,

    NETSETUP_STATE_JOINING   = 4,
    NETSETUP_STATE_VERIFYING = 5,
    NETSETUP_STATE_NAMED     = 6,

    NETSETUP_STATE_ACCEPTING = 7,
    NETSETUP_STATE_OFFERED   = 7,

    NETSETUP_STATE_CHOOSING = 8,

    // Four readings of one state, and the last is a threshold rather than a name for it.
    NETSETUP_STATE_POSITION       = 9,
    NETSETUP_STATE_HOST_CHOSEN    = 9,
    NETSETUP_STATE_WAITING_A      = 9,
    NETSETUP_STATE_AFTER_CHOOSING = 9,

    NETSETUP_STATE_READY  = 10,
    NETSETUP_STATE_VERIFY = 10,

    // The last state the setup dialogues run in, used as a bound.
    NETSETUP_STATE_DIALOG_LAST = 11,

    NETSETUP_STATE_STARTED = 12,
    NETSETUP_STATE_PLAYING = 13
};

// Not a state: the window message the name arrival posts.
#define NETSETUP_NAME_ARRIVED 0x4d3

enum NetSetupDialogId
{
    // The position dialog's Start and Ready buttons; its Cancel is IDCANCEL.
    ID_START           = IDOK,
    ID_READY           = 3,
    ID_JOIN            = 0x3ef,
    ID_HOST            = 0x408,
    ID_SEARCH          = 0x409,
    ID_SESSION_LIST    = 0x42c,
    ID_PROVIDER_LIST   = 0x42d,
    ID_POSITION_FIRST  = 0x4c1,
    ID_POSITION_SECOND = 0x4c2,
    ID_POSITION_THIRD  = 0x4c3,
    ID_POSITION_FOURTH = 0x4c4,
    ID_CHAT_ENTRY      = 0x4c6,
    ID_PLAYER_LIST     = 0x4d3,
    ID_TEAM_HOME       = 0x4d4,
    ID_TEAM_AWAY       = 0x4d5,
    ID_ARENA           = 0x4d6,
    // The chat list beside ID_CHAT_ENTRY.
    ID_CHAT_LIST = 0x4d7
};

// The four asset kinds the network sync sends and receives.
enum AssetType
{
    ASSET_TYPE_SETTINGS = 2,
    ASSET_TYPE_TEAMS    = 3,
    ASSET_TYPE_ARENAS   = 4,
    ASSET_TYPE_PLANS    = 5
};

#endif
