#ifndef HYPERBLADE_PLANFILE_H
#define HYPERBLADE_PLANFILE_H

#include <cstdio>
#include <memory>
#include <vector>

union PlanWord;

namespace PlanFile
{

struct Blobs
{
    std::unique_ptr<PlanWord[]> pShipped;
    // Null when PlanPatch has no fixes for the plan.
    std::unique_ptr<PlanWord[]> pFixed;
};

std::vector<int> ReadBinary(FILE* pFile);
Blobs            OpenBinary(const char* pszName);

} // namespace PlanFile

#endif
