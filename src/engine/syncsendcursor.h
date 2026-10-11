#ifndef HYPERBLADE_SYNCSENDCURSOR_H
#define HYPERBLADE_SYNCSENDCURSOR_H

// Which sync message the Rok posts next: the scores while the low half is nought, then the
// possession row the high half names.
struct SyncSendCursor
{
    unsigned int nWord;

    short GetPossessionTurn() const
    {
        return (short)nWord;
    }

    int GetRow() const
    {
        return (int)(nWord >> 16);
    }

    void SetPossessionTurn()
    {
        nWord |= 1;
    }

    // Hands the turn back to the scores.
    void SetRow(int nRow)
    {
        nWord = nRow << 16;
    }
};

#endif
