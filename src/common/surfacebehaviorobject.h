#ifndef HYPERBLADE_SURFACEBEHAVIOROBJECT_H
#define HYPERBLADE_SURFACEBEHAVIOROBJECT_H

#include "planvm/gamebehaviorobject.h"

// Slot three, which the base leaves pure; all nine surface behaviours share this body.
class SurfaceBehaviorObject : public GameBehaviorObject
{
public:
    void StartMotion() override;
};

#endif
