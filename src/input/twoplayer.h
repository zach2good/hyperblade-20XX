// 2 Player Mode, the original menu's unhandled item: two humans on one machine, one per team,
// sharing the camera, which follows player one.
//
// The engine has one keyboard device and one view owner, and the plans read both. So player two's
// held keys, sticky bindings and dromer are kept aside, and swapped into the device and
// m_nViewedSlot whenever the engine runs player two's team's dromers, or player two's input.

#ifndef HYPERBLADE_TWOPLAYER_H
#define HYPERBLADE_TWOPLAYER_H

#include <SDL3/SDL.h>

class EngineObject;

namespace TwoPlayer
{

bool On();

// Takes effect at the next restart. Refused in a network match and under autopilot.
bool Set(bool bOn);

// A new period: player two takes the first dromer of the team opposite player one.
void Reset();

// Whether player two drives the slot's plan: a dromer on player two's team.
bool IsSecondPlayers(int nSlot);

// The keyboard drives player one. One pad drives player two; with two, the first is player one's.
bool PadIsSecond(SDL_JoystickID nPad);

// Player two's key, through the same path the keyboard's keys take.
void KeyDown(unsigned int nKey);
void KeyUp(unsigned int nKey);

// Player two's held-key timeouts, which the keyboard device runs for player one each tick.
void CheckSecondTimeouts();

// Points m_apTracked at the slot: its own team's entry alone in 2 Player Mode, both otherwise.
void Track(int nSlot);

// While alive, the engine sees the object's player as the one at the keys.
class Scope
{
public:
    explicit Scope(EngineObject* pObject);
    ~Scope();

    Scope(const Scope&)            = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&&)                 = delete;
    Scope& operator=(Scope&&)      = delete;

private:
    bool m_bSwapped = false;
};

} // namespace TwoPlayer

#endif
