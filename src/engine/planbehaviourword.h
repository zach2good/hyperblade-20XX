#ifndef HYPERBLADE_PLANBEHAVIOURWORD_H
#define HYPERBLADE_PLANBEHAVIOURWORD_H

// kCommandSetPlanAndBehavior's payload: the plan in the high half and the behaviour in the low.
struct PlanBehaviourWord
{
    static short GetPlan(unsigned int nWord)
    {
        return static_cast<short>(nWord >> 16);
    }

    static short GetBehaviour(unsigned int nWord)
    {
        return static_cast<short>(nWord);
    }

    static unsigned int Pack(int nPlan, unsigned int nBehaviour)
    {
        return (nPlan << 16) | nBehaviour;
    }
};

#endif
