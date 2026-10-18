#ifndef HYPERBLADE_NETSEQUENCEWORD_H
#define HYPERBLADE_NETSEQUENCEWORD_H

// NetMoveBody::nSequence: the sender's slot in the low half; above it the Rok's carrier plus one,
// or the referee's two target slots.
struct NetSequenceWord
{
    // The client masks a short of the shifted word, at 0x0047132b and 0x00471368.
    static int GetTarget(unsigned int nWord)
    {
        return static_cast<short>(nWord >> 16) & 0xff;
    }

    static int GetSecondTarget(unsigned int nWord)
    {
        return static_cast<short>(nWord >> 24) & 0xff;
    }

    static unsigned int GetCarrier(unsigned int nWord)
    {
        return nWord >> 16;
    }

    static int PackTarget(int nTarget)
    {
        return nTarget << 16;
    }

    static int PackCarrier(int nCarrier)
    {
        return (nCarrier + 1) << 16;
    }
};

#endif
