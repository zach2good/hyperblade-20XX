// v1.0.0.0's GDI debug overlays: dromer energy, polygon counts, two message logs and queued text.

#ifdef HYPERX_V100
#ifndef HYPERBLADE_DEBUGOVERLAY_H
#define HYPERBLADE_DEBUGOVERLAY_H

#include "common/compat.h"

// One ExtTextOutA call, field for field.
struct TextOutRecord
{
    LPCSTR   pszText;  // 0x00
    UINT     nLength;  // 0x04
    int      nX;       // 0x08
    int      nY;       // 0x0c
    COLORREF clrText;  // 0x10
    UINT     nOptions; // 0x14
    RECT*    pRect;    // 0x18
    INT*     pnDx;     // 0x1c
};

namespace DebugOverlay
{

void __cdecl DrawDromerEnergy(HDC hDC);
void __cdecl DrawPolyCounts(HDC hDC);

void __cdecl DrawLogs(HDC hDC);
// Numbers the line; the copy reads six bytes past the text, the length of the number prefix.
void __cdecl AddLogLine(const char* pszText, short nLength);

TextOutRecord* __cdecl PopText(void);
char __cdecl           PushText(TextOutRecord* pRecord);
char __cdecl           PopTextBatch(TextOutRecord** ppRecords, short* pnCount);
char __cdecl           PushTextBatch(TextOutRecord* pRecords, short nCount);

void __cdecl DrawQueuedText(HDC hDC);
void __cdecl DrawQueuedBatches(HDC hDC);

} // namespace DebugOverlay

#endif
#endif
