#include "common/compat.h"
#include "planvm/gamebehaviorobject.h"
#include "engine/devicector.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "planvm.h"
#include "ui/screens/leaguesetupuiscreen.h"
#include "ui/torchbitmaps.h"
#include <windows.h>
#include "engine/engineobject.h"
#include "planvm/planeditor.h"
#include "planvm/planeditordialog.h"

#define NODE_LIST_ITEM 0x430

// FUNCTION: HYPERX 0x0047c550
void PlanEditor::ApplyBehaviorSelection(HWND hDlg, HWND hControl)
{
    const short nCamera = PlanEditorDialog::ApplyCameraSelection(hDlg);

    PlanVM* const pText = PlanEditorDialog::LoadPlanText(
        hDlg, (short)PlanEditorDialog::GetBehaviorSel(hDlg), nCamera);

    pText->ParseHeaderDeclarations();

    SendMessageA(hControl, CB_RESETCONTENT, 0, 0);

    short nIndex = 1;
    char* pName  = pText->m_aBehaviorNames[0];

    while (*pName != 0)
    {
        SendMessageA(hControl, CB_ADDSTRING, 0, (LPARAM)pName);

        pName = pText->m_aBehaviorNames[nIndex++];
    }

    const char* pBehaviour = g_pGameEngine->m_ppSlots[nCamera]->GetPlanState()->m_pCurrent->m_pszName;

    EngineObject* const pObject = g_pGameEngine->m_ppSlots[nCamera];

    SendMessageA(hControl, CB_SETCURSEL, pObject->GetBehaviorIndex(pObject->GetPlanState(), pBehaviour), 0);

    PlanEditor::ApplyNodeSelection(hDlg, GetDlgItem(hDlg, NODE_LIST_ITEM));
}

#define NODE_CONDITION_STRIDE 20

// One index runs through both loops unreset, so the second block starts one past the first.
// FUNCTION: HYPERX 0x0047c640
void PlanEditor::ApplyNodeSelection(HWND hDlg, HWND hControl)
{
    PlanVM* const pPlan = (PlanVM*)GetWindowLongA(hDlg, GWL_USERDATA);

    SendMessageA(hControl, CB_RESETCONTENT, 0, 0);

    short nIndex = 1;
    char* pEntry = pPlan->m_aMessageNames[0];

    while (*pEntry != 0)
    {
        SendMessageA(hControl, CB_ADDSTRING, 0, (LPARAM)pEntry);

        pEntry = pPlan->m_aMessageNames[nIndex];
        nIndex++;
    }

    const int nBase = (short)(PlanEditorDialog::GetNodeSel(hDlg) + 1) * NODE_CONDITION_STRIDE;

    pEntry = pPlan->m_aMessageNames[nIndex + nBase];
    nIndex++;

    while (*pEntry != 0)
    {
        SendMessageA(hControl, CB_ADDSTRING, 0, (LPARAM)pEntry);

        pEntry = pPlan->m_aMessageNames[nIndex + nBase];
        nIndex++;
    }

    SendMessageA(hControl, CB_SETCURSEL, 0, 0);
}
