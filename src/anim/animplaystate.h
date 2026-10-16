#ifndef HYPERBLADE_ANIMPLAYSTATE_H
#define HYPERBLADE_ANIMPLAYSTATE_H

// The play states, as DATA/ENUMIDS.H's PlayState numbers them.
#define AA_DONE       0
#define AA_PLAY       1
#define AA_PAUSE      2
#define AA_SINGLESTEP 3

// The play state AnimPlayer keeps twice: live, and the copy SetClip saves before it switches.
struct AnimPlayState
{
    short wClipIndex; // 0x00
    // Three parallel arrays of ten: event target, message and parameter per channel.
    int                  anEventTarget[10];  // 0x04
    int                  anEventMessage[10]; // 0x2c
    int                  anEventParam[10];   // 0x54
    short                wEventCursor;       // 0x7c
    short                wEventTrigger;      // 0x7e
    class AnimClipEntry* pEventRecord;       // 0x80
    char                 bChannelPending;    // 0x84
    int                  nHold;              // 0x88
    // A_FORWARD or A_REVERSE.
    int nDirection; // 0x8c
    int nPlayState; // 0x90
    // Cleared and copied with the record, and never read or written by name.
    short wUnused; // 0x94
    short wLoops;  // 0x96
    // The engine slot the clip-end event goes to.
    int   nSlot;          // 0x98
    int   nArgA;          // 0x9c
    int   nArgB;          // 0xa0
    int   nFrame;         // 0xa4
    float flPosition;     // 0xa8
    float flStep;         // 0xac
    int   nLoopFrame;     // 0xb0
    int   nEndFrame;      // 0xb4
    float flPlaybackRate; // 0xb8
    // A per-clip duration cache, indexed like the set's clip table.
    float aflClipDuration[6]; // 0xbc
    // Set when the next frame must decode the whole pose rather than the changed bones.
    char bFullDecode; // 0xd4
    // Cleared and copied with the record, and never read or written by name.
    int nUnused; // 0xd8
};

static_assert(sizeof(AnimPlayState) == 0xdc);

#endif
