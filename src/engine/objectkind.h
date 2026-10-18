#ifndef HYPERBLADE_OBJECTKIND_H
#define HYPERBLADE_OBJECTKIND_H

// The kinds Cmd_DefineObject's type table gives each type keyword, which CreateObject switches on
// and EngineObject::m_nKind keeps. Both goals share one kind and both cameras another; SoundMan
// shares Generic's, and no keyword gives 0x0e.
enum class ObjectKind
{
    kNull            = 0x00,
    kDromer          = 0x01,
    kRok             = 0x02,
    kDrome           = 0x03,
    kRamp            = 0x04,
    kGoal            = 0x05,
    kCamera          = 0x06,
    kStartGun        = 0x07,
    kTurnstile       = 0x08,
    kMultiplier      = 0x09,
    kLaserHurdle     = 0x0a,
    kGreenRandomizer = 0x0b,
    kRedRandomizer   = 0x0c,
    kGod             = 0x0d,
    kLimboPole       = 0x0f,
    kHurdle          = 0x10,
    kFastLane        = 0x11,
    kExplosion       = 0x12,
    kArmory          = 0x13,
    kKeyboard        = 0x14,
    kGamepad         = 0x15,
    kJoystick        = 0x16,
    kMouse           = 0x17,
    kGeneric         = 0x18,
    kSlowLane        = 0x19,
    kMine            = 0x1a,
    kSwarf           = 0x1b,
    kCoach           = 0x1c,
    kBodyPart        = 0x1d,
};

#endif
