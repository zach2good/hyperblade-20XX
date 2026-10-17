#ifndef HYPERBLADE_PLANFILE_H
#define HYPERBLADE_PLANFILE_H

#include <cstdio>

union PlanWord;

namespace PlanFile
{

PlanWord* ReadBinary(FILE* pFile, unsigned int* pnCount);
PlanWord* OpenBinary(const char* pszName, void** ppUnused);

} // namespace PlanFile

#endif
