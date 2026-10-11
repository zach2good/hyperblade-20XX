#ifndef HYPERBLADE_PLANEDITORDIALOG_H
#define HYPERBLADE_PLANEDITORDIALOG_H

#include "common/compat.h"

namespace PlanEditorDialog
{

BOOL CALLBACK         Proc(HWND hDlg, UINT nMsg, UINT wParam, LONG lParam);
short                 ApplyCameraSelection(HWND hDlg);
LRESULT               GetBehaviorSel(HWND hDlg);
LRESULT               GetNodeSel(HWND hDlg);
LRESULT               GetCondSel(HWND hDlg);
int                   PopulateCameraList(HWND hDlg);
void                  PopulateBehaviorList(HWND hDlg, short nCamera);
int                   OnInit(HWND hDlg, HWND hwndFocus, LPARAM lParam);
int                   OnCommand(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnClose(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnLoadPlan(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnCameraChanged(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnBehaviorChanged(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnNodeChanged(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnCompilePlan(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnCompileAll(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnActivateBehavior(HWND hDlg, int nId, HWND hwndCtl, int nCode);
int                   OnTriggerCondition(HWND hDlg, int nId, HWND hwndCtl, int nCode);
class PlanVM* __cdecl LoadPlanText(HWND hDlg, short nBehavior, short nCamera);
inline char*          PlanName(class EngineObject* pObject, short nIndex);

} // namespace PlanEditorDialog

#endif
