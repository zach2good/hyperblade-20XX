// The exception the plan loaders catch; its RTTI descriptor spells .?AUAdlibError@@.
// Nothing in the image throws one: the compiler and loader throw char* messages, so the
// AdlibError handlers never run.

#ifndef HYPERBLADE_ADLIBERROR_H
#define HYPERBLADE_ADLIBERROR_H

struct AdlibError
{
    int   nCode;
    char* pszText;
};

#endif
