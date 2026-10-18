#ifndef HYPERBLADE_OBJECTPHASE_H
#define HYPERBLADE_OBJECTPHASE_H

// Who drives an object: this machine, or the moves a network player sends.
enum class ObjectPhase
{
    kLocal = 1,
    // EngineObject::InitFields leaves it and the constructor replaces it.
    kBuilding = 2,
    kNetwork  = 3
};

#endif
