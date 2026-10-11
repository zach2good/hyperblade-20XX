#ifndef HYPERBLADE_PLANFILE_H
#define HYPERBLADE_PLANFILE_H

struct _iobuf;
union PlanWord;

namespace PlanFile
{

PlanWord*         ReadBinary(struct _iobuf* pFile, unsigned int* pnCount);
PlanWord* __cdecl OpenBinary(const char* pszName, void** ppUnused);

} // namespace PlanFile

#endif
