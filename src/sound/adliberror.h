// The exception the plan loaders catch; its RTTI descriptor spells .?AUAdlibError@@.

#ifndef HYPERBLADE_ADLIBERROR_H
#define HYPERBLADE_ADLIBERROR_H

struct AdlibError
{
    int   nCode;
    char* pszText;
};

#endif
