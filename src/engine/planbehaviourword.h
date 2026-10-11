#ifndef HYPERBLADE_PLANBEHAVIOURWORD_H
#define HYPERBLADE_PLANBEHAVIOURWORD_H

// COMMAND_SET_PLAN_AND_BEHAVIOR's payload: the plan in the high half and the behaviour in the low.
struct PlanBehaviourWord
{
    static short GetPlan(unsigned int nWord)
    {
        return (short)(nWord >> 16);
    }

    static short GetBehaviour(unsigned int nWord)
    {
        return (short)nWord;
    }

    static unsigned int Pack(int nPlan, unsigned int nBehaviour)
    {
        return (nPlan << 16) | nBehaviour;
    }
};

#endif
