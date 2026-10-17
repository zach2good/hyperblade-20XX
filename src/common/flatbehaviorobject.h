#ifndef HYPERBLADE_FLATBEHAVIOROBJECT_H
#define HYPERBLADE_FLATBEHAVIOROBJECT_H

#include "planvm/gamebehaviorobject.h"

class FlatBehaviorObject : public GameBehaviorObject
{
public:
    void StartMotion() override;
};

#endif
