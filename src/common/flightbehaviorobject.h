#ifndef HYPERBLADE_FLIGHTBEHAVIOROBJECT_H
#define HYPERBLADE_FLIGHTBEHAVIOROBJECT_H

#include "planvm/gamebehaviorobject.h"

class FlightBehaviorObject : public GameBehaviorObject
{
public:
    void StartMotion() override;
};

#endif
