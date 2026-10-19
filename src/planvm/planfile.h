#ifndef HYPERBLADE_PLANFILE_H
#define HYPERBLADE_PLANFILE_H

#include <cstdio>
#include <memory>

union PlanWord;

namespace PlanFile
{

std::unique_ptr<PlanWord[]> ReadBinary(FILE* pFile, unsigned int* pnCount);
std::unique_ptr<PlanWord[]> OpenBinary(const char* pszName, void** ppUnused);

} // namespace PlanFile

#endif
