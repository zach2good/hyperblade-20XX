// Globals the input and cheat code reads; src/globals.cpp defines them in .data order.

#ifndef HYPERBLADE_INPUTGLOBALS_H
#define HYPERBLADE_INPUTGLOBALS_H
#include "common/compat.h"

#include "globals.h"
#include <stddef.h>
class CheatState;

#define KEY_BINDING_COUNT 10

// The ten key bindings: ini name, default, and what was read.
// GLOBAL: HYPERX 0x004c41e0
extern const char* g_apszKeyBindingNames[KEY_BINDING_COUNT];

// The bound keys' slots in g_anKeyBindingDefaults that the readers name.
#define BINDING_DOWN   1
#define BINDING_LEFT   2
#define BINDING_RIGHT  3
#define BINDING_ATTACK 4
#define BINDING_THROW  8
#define BINDING_JUMP   9

// GLOBAL: HYPERX 0x004c4398
extern unsigned int g_anKeyBindingDefaults[KEY_BINDING_COUNT];

// The identity with four and five exchanged; only the binding commit reads it.
// GLOBAL: HYPERX 0x004c43c0
extern int g_anKeyBindingOrder[KEY_BINDING_COUNT];

// GLOBAL: HYPERX 0x004c43e8
extern unsigned int g_anKeyBindings[KEY_BINDING_COUNT];

// Which of the two cameras the tab key last cycled to.
// GLOBAL: HYPERX 0x004c4d98
extern short g_nTabCycle;

// GLOBAL: HYPERX 0x004ce998
extern CheatState g_cheatState;

// GLOBAL: HYPERX 0x004c8584
extern unsigned int g_nLastEscState;

// One record: the restore cheat and Object::ScaleBones bound their walks on the next field.
struct TeamScaleSlots
{
    int   anTeamOrder[4];
    int   anScaleTargetSlots[4];
    float flShrinkScaleX;
    float flShrinkScaleY;
    float flShrinkScaleZ;
};

static_assert(offsetof(TeamScaleSlots, anScaleTargetSlots) == 0x10);
static_assert(offsetof(TeamScaleSlots, flShrinkScaleX) == 0x20);

// GLOBAL: HYPERX 0x004c2948
extern TeamScaleSlots g_teamScaleSlots;

// GLOBAL: HYPERX 0x004c2974
extern int g_anShrinkBones[15];

// GLOBAL: HYPERX 0x004c29b0
extern float g_flGrowScaleX;

// GLOBAL: HYPERX 0x004c29b4
extern float g_flGrowScaleY;

// GLOBAL: HYPERX 0x004c29b8
extern float g_flGrowScaleZ;

// GLOBAL: HYPERX 0x004c29bc
extern int g_anGrowBones[15];

// GLOBAL: HYPERX 0x004c29f8
extern float g_flMirrorScaleX;

// GLOBAL: HYPERX 0x004c29fc
extern float g_flMirrorScaleY;

// GLOBAL: HYPERX 0x004c2a00
extern float g_flMirrorScaleZ;

// GLOBAL: HYPERX 0x004c2a04
extern int g_anMirrorBones[15];

// GLOBAL: HYPERX 0x004c2a40
extern float g_flStretchScaleX;

// GLOBAL: HYPERX 0x004c2a44
extern float g_flStretchScaleY;

// GLOBAL: HYPERX 0x004c2a48
extern float g_flStretchScaleZ;

// GLOBAL: HYPERX 0x004c2a4c
extern int g_anStretchBones[15];

// GLOBAL: HYPERX 0x004c2a88
extern float g_flFlipScaleX;

// GLOBAL: HYPERX 0x004c2a8c
extern float g_flFlipScaleY;

// GLOBAL: HYPERX 0x004c2a90
extern float g_flFlipScaleZ;

// GLOBAL: HYPERX 0x004c2a94
extern int g_anFlipBones[15];

// GLOBAL: HYPERX 0x004c2ad0
extern float g_flBulkUpScaleX;

// GLOBAL: HYPERX 0x004c2ad4
extern float g_flBulkUpScaleY;

// GLOBAL: HYPERX 0x004c2ad8
extern float g_flBulkUpScaleZ;

// GLOBAL: HYPERX 0x004c2adc
extern int g_anBulkUpBodyBones[10];

// GLOBAL: HYPERX 0x004c2b04
extern int g_anBulkUpHandBones[2];

// GLOBAL: HYPERX 0x004c2b0c
extern int g_anBulkUpHeadFootBones[3];

// GLOBAL: HYPERX 0x004c2b18
extern float g_flSquashScaleX;

// GLOBAL: HYPERX 0x004c2b1c
extern float g_flSquashScaleY;

// GLOBAL: HYPERX 0x004c2b20
extern float g_flSquashScaleZ;

// GLOBAL: HYPERX 0x004c2b24
extern int g_anSquashBodyBones[10];

// GLOBAL: HYPERX 0x004c2b4c
extern int g_anSquashExtremityBones[5];

// GLOBAL: HYPERX 0x004c2b60
extern float g_flFlipLegsScaleX;

// GLOBAL: HYPERX 0x004c2b64
extern float g_flFlipLegsScaleY;

// GLOBAL: HYPERX 0x004c2b68
extern float g_flFlipLegsScaleZ;

// GLOBAL: HYPERX 0x004c2b6c
extern int g_anFlipLegsBodyBones[6];

// GLOBAL: HYPERX 0x004c2b84
extern int g_anFlipLegsExtremityBones[5];

#endif
