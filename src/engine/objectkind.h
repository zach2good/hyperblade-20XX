#ifndef HYPERBLADE_OBJECTKIND_H
#define HYPERBLADE_OBJECTKIND_H

// The kinds Cmd_DefineObject's type table gives each type keyword, which CreateObject switches on
// and EngineObject::m_nKind keeps. Both goals share one kind and both cameras another; SoundMan
// shares Generic's, and no keyword gives 0x0e.
enum ObjectKind
{
    KIND_NULL             = 0x00,
    KIND_DROMER           = 0x01,
    KIND_ROK              = 0x02,
    KIND_DROME            = 0x03,
    KIND_RAMP             = 0x04,
    KIND_GOAL             = 0x05,
    KIND_CAMERA           = 0x06,
    KIND_START_GUN        = 0x07,
    KIND_TURNSTILE        = 0x08,
    KIND_MULTIPLIER       = 0x09,
    KIND_LASER_HURDLE     = 0x0a,
    KIND_GREEN_RANDOMIZER = 0x0b,
    KIND_RED_RANDOMIZER   = 0x0c,
    KIND_GOD              = 0x0d,
    KIND_LIMBO_POLE       = 0x0f,
    KIND_HURDLE           = 0x10,
    KIND_FAST_LANE        = 0x11,
    KIND_EXPLOSION        = 0x12,
    KIND_ARMORY           = 0x13,
    KIND_KEYBOARD         = 0x14,
    KIND_GAMEPAD          = 0x15,
    KIND_JOYSTICK         = 0x16,
    KIND_MOUSE            = 0x17,
    KIND_GENERIC          = 0x18,
    KIND_SLOW_LANE        = 0x19,
    KIND_MINE             = 0x1a,
    KIND_SWARF            = 0x1b,
    KIND_COACH            = 0x1c,
    KIND_BODY_PART        = 0x1d,
};

#endif
