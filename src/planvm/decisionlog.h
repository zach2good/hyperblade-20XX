#ifndef HYPERBLADE_DECISIONLOG_H
#define HYPERBLADE_DECISIONLOG_H

// A local object records the branches its decisions take; its network copy replays them oldest first.
constexpr auto kDecisionLogRecording = 0x8000;
// How many branches the word holds; each count packs them at its own widths.
constexpr auto kDecisionLogCount = 0x3000;
constexpr auto kDecisionLogOne   = 0x1000;
constexpr auto kDecisionLogTwo   = 0x2000;
// What a recording caller starts from.
constexpr auto kDecisionLogFresh = -1;

// One branch in bits 0-11; two in 0-6 and 7-12; three in 0-4, 5-8 and 9-11.
struct DecisionLog
{
    static int IsRecording(short nWord)
    {
        return nWord & kDecisionLogRecording;
    }

    static void Push(short* pWord, short nBranch)
    {
        if (*pWord == kDecisionLogFresh)
        {
            *pWord = 0;
        }

        const short nWord = *pWord;

        if ((nWord & kDecisionLogCount) == 0)
        {
            *pWord = static_cast<short>((nWord & 0xfff) | nBranch | kDecisionLogRecording | kDecisionLogOne);
        }
        else if ((nWord & kDecisionLogCount) == kDecisionLogOne)
        {
            *pWord = static_cast<short>((static_cast<unsigned int>(nBranch) << 7) | (nWord & 0x7f) | kDecisionLogRecording | kDecisionLogTwo);
        }
        else
        {
            // The third branch is masked where it should be shifted, so its replay takes bits 9-11 (0x0045d963).
            *pWord = static_cast<short>(((nWord >> 2) & 0x1e0) | (nWord & 0x1f) | (nBranch & 0xe00) | kDecisionLogRecording | kDecisionLogCount);
        }
    }

    // Moves the oldest branch into pBranch.
    static void Pop(short* pWord, short* pBranch)
    {
        const short nWord = *pWord;

        if ((nWord & kDecisionLogCount) == kDecisionLogOne)
        {
            *pBranch = static_cast<short>(nWord & 0xfff);

            *pWord = 0;
        }
        else if ((nWord & kDecisionLogCount) == kDecisionLogTwo)
        {
            *pBranch = static_cast<short>(nWord & 0x7f);

            *pWord = static_cast<short>(((nWord >> 7) & ~0x1e0) | kDecisionLogOne);
        }
        else
        {
            *pBranch = static_cast<short>(nWord & 0x1f);

            *pWord = static_cast<short>((static_cast<short>(((nWord >> 3) & 0x3c) | (nWord & 0xe00)) >> 2) | kDecisionLogTwo);
        }
    }
};

#endif
