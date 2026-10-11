#ifndef HYPERBLADE_DECISIONLOG_H
#define HYPERBLADE_DECISIONLOG_H

// A local object records the branches its decisions take; its network copy replays them oldest first.
#define DECISION_LOG_RECORDING 0x8000
// How many branches the word holds; each count packs them at its own widths.
#define DECISION_LOG_COUNT 0x3000
#define DECISION_LOG_ONE   0x1000
#define DECISION_LOG_TWO   0x2000
// What a recording caller starts from.
#define DECISION_LOG_FRESH -1

// One branch in bits 0-11; two in 0-6 and 7-12; three in 0-4, 5-8 and 9-11.
struct DecisionLog
{
    static int IsRecording(short nWord)
    {
        return nWord & DECISION_LOG_RECORDING;
    }

    static void Push(short* pWord, short nBranch)
    {
        if (*pWord == DECISION_LOG_FRESH)
        {
            *pWord = 0;
        }

        const short nWord = *pWord;

        if ((nWord & DECISION_LOG_COUNT) == 0)
        {
            *pWord = (short)((nWord & 0xfff) | nBranch | DECISION_LOG_RECORDING | DECISION_LOG_ONE);
        }
        else if ((nWord & DECISION_LOG_COUNT) == DECISION_LOG_ONE)
        {
            *pWord = (short)(((unsigned int)nBranch << 7) | (nWord & 0x7f) | DECISION_LOG_RECORDING | DECISION_LOG_TWO);
        }
        else
        {
            // The third branch is masked where it should be shifted, so its replay takes bits 9-11 (0x0045d963).
            *pWord = (short)(((nWord >> 2) & 0x1e0) | (nWord & 0x1f) | (nBranch & 0xe00) | DECISION_LOG_RECORDING | DECISION_LOG_COUNT);
        }
    }

    // Moves the oldest branch into pBranch.
    static void Pop(short* pWord, short* pBranch)
    {
        const short nWord = *pWord;

        if ((nWord & DECISION_LOG_COUNT) == DECISION_LOG_ONE)
        {
            *pBranch = (short)(nWord & 0xfff);

            *pWord = 0;
        }
        else if ((nWord & DECISION_LOG_COUNT) == DECISION_LOG_TWO)
        {
            *pBranch = (short)(nWord & 0x7f);

            *pWord = (short)(((nWord >> 7) & ~0x1e0) | DECISION_LOG_ONE);
        }
        else
        {
            *pBranch = (short)(nWord & 0x1f);

            *pWord = (short)(((short)(((nWord >> 3) & 0x3c) | (nWord & 0xe00)) >> 2) | DECISION_LOG_TWO);
        }
    }
};

#endif
