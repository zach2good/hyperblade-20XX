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

void BeforeWindow(void);
void ChooseDisplay(void);
void OverrideDisplayMode(int* pnWidth, int* pnHeight);
void HoldSplash(void* hWindow);
void ForceStartState(void);
void ForceModelSet(int* pbTexEnabled);
void BeforeFrame(void);
void AfterFrame(void* hWindow);

void PresentFrame(void* hWindowDC, void* hBackDC, int cx, int cy);
void ClearSurround(void* hWindow, int cx, int cy);
void OnEnterSizeMove(void* hWindow);
void OnExitSizeMove(void* hWindow);
int  OnTimer(void* hWindow, unsigned int nTimerId);
int  FocusChanged(unsigned int bActive);
void NoteKey(unsigned int nMessage, unsigned int nKey);
void ReleaseHeldKeys(void);

unsigned int ColourFromMasks(unsigned int nColour, unsigned int nRMask, unsigned int nGMask, unsigned int nBMask);
unsigned int CornerColourKey(IDirectDrawSurface* pSurface, unsigned int nRMask, unsigned int nGMask, unsigned int nBMask);
int          NearestSoftPaletteIndex(unsigned int nColour);
int          FaceUnprojectable(void);
int          EdgeOffScreen(float flRoundY1, float flRoundY2);

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
