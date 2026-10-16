// The bring-up entry points, declared always; only the bring-up build links their bodies.

#ifndef HYPERBLADE_BRINGUP_H
#define HYPERBLADE_BRINGUP_H

struct IDirectDrawSurface;

namespace BringUp
{

void Trace(const char* pszWhat, int nValue);
void TraceText(const char* pszWhat, const char* pszValue);
void TraceFloat(const char* pszWhat, float flValue);
void TraceChange(const char* pszWhat, int nValue);
void TraceVec3(const char* pszWhat, float x, float y, float z);
int  Now(void);

void SaveBitmap(const char* pszWhat, void* hBitmap);
void SaveWindowDC(const char* pszWhat, void* hDeviceContext, int nWidth, int nHeight);
void SaveD3DFrame(const char* pszWhat);

void SelectResolution(void);
void OverrideDisplayMode(int* pnWidth, int* pnHeight);
void HoldSplash(void* hWindow);
void ForceStartState(void);
void ForceModelSet(int* pbTexEnabled);
void BeforeFrame(void);
void AfterFrame(void* hWindow);

int FocusChanged(unsigned int bActive);

void PickFixture(void);
void ForceScores(int* pnHome, int* pnAway);
void ShortenOvertime(int* pnOvertimeMs);
int  ConfirmQuit(void);
void OnFatalSkipped(void);

void CountPlanMessage(int nEvent);
void DumpPlanMessages(void);
void TraceScene(struct GameEngine* pEngine);
void TraceMessagePosted(const char* pszTo, int nKind, int nWanted);
void TraceMessageDispatched(const char* pszTo, int nKind, int nSlot, int nPhase);
void TraceTimerStep(const char* pszOwner, int bDead, int nDeadline, int nMessage);

} // namespace BringUp

#endif
