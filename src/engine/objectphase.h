#ifndef HYPERBLADE_OBJECTPHASE_H
#define HYPERBLADE_OBJECTPHASE_H

// Who drives an object: this machine, or the moves a network player sends.
enum ObjectPhase
{
    OBJECT_PHASE_LOCAL = 1,
    // EngineObject::InitFields leaves it and the constructor replaces it.
    OBJECT_PHASE_BUILDING = 2,
    OBJECT_PHASE_NETWORK  = 3
};

#endif
