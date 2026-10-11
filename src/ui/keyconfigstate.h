#ifndef HYPERBLADE_KEYCONFIGSTATE_H
#define HYPERBLADE_KEYCONFIGSTATE_H

// The table's marker is in src/ui/screens/keyconfigscreen.cpp.
struct KeyBitmapEntry
{
    unsigned int nKey;
    const char*  pszBitmap;
};

// The block the sheet hangs off its window at GWL_USERDATA.
struct KeyConfigState
{
    struct KeyRow* pSaved;
    short          nSource;
    short          nUnused;
    char*          pszPrimary;
    char*          pszAlternate;
};

#endif
