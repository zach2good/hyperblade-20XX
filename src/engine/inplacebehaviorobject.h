#ifndef HYPERBLADE_INPLACEBEHAVIOROBJECT_H
#define HYPERBLADE_INPLACEBEHAVIOROBJECT_H

#include "planvm/gamebehaviorobject.h"

class InPlaceBehaviorObject : public GameBehaviorObject
{
public:
    void StartMotion() override;
};

#endif
