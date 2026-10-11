#include "common/compat.h"
#include "engine/devicector.h"
#include "planvm.h"
#include "globals.h"
#include "planvmglobals.h"
#include "engine/gameengine.h"
#include "sound/adliberror.h"
#include "common/pathstring.h"
#include <mbstring.h>
#include "app/gameapp.h"
#include "engine/engineobject.h"
#include "planvm/planeditordialog.h"
#include "planvm/gamebehaviorobject.h"
#include "app/win32.h"
#include "planfile.h"
#include "planeditor.h"
#include "plannode.h"

#define IDC_PLAN_LOAD        0x408
#define IDC_PLAN_CAMERA      0x41f
#define IDC_PLAN_BEHAVIOR    0x420
#define IDC_PLAN_COMPILE     0x421
#define IDC_PLAN_COMPILE_ALL 0x422
#define IDC_PLAN_NODE        0x42f
#define IDC_PLAN_CONDITION   0x430
#define IDC_PLAN_ACTIVATE    0x432
#define IDC_PLAN_TRIGGER     0x433

// A slot past the object's own plans answers the default name.
inline char* PlanEditorDialog::PlanName(EngineObject* pObject, short nIndex)
{
    if (nIndex < pObject->m_nPlanCount)
    {
        return pObject->m_papszPlanNames[nIndex];
    }

    return g_pszDefaultPlanName;
}

// FUNCTION: HYPERX 0x0047b9a0
short PlanEditorDialog::ApplyCameraSelection(HWND hDlg)
{
    EngineObject* pObject;

    pObject = g_pGameEngine->m_ppObjects
                  [(short)SendMessageA(GetDlgItem(hDlg, IDC_PLAN_CAMERA), CB_GETCURSEL, 0, 0)];

    return pObject->GetSlot();
}

// FUNCTION: HYPERX 0x0047b9e0
LRESULT PlanEditorDialog::GetBehaviorSel(HWND hDlg)
{
    return SendMessageA(GetDlgItem(hDlg, IDC_PLAN_BEHAVIOR), CB_GETCURSEL, 0, 0);
}

// FUNCTION: HYPERX 0x0047ba10
LRESULT PlanEditorDialog::GetNodeSel(HWND hDlg)
{
    return SendMessageA(GetDlgItem(hDlg, IDC_PLAN_NODE), CB_GETCURSEL, 0, 0);
}

// FUNCTION: HYPERX 0x0047ba40
LRESULT PlanEditorDialog::GetCondSel(HWND hDlg)
{
    return SendMessageA(GetDlgItem(hDlg, IDC_PLAN_CONDITION), CB_GETCURSEL, 0, 0);
}

// FUNCTION: HYPERX 0x0047ba70
BOOL CALLBACK PlanEditorDialog::Proc(HWND hDlg, UINT nMsg, UINT wParam, LONG lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
            return (BOOL)PlanEditorDialog::OnInit(hDlg, (HWND)wParam, lParam);

        case WM_COMMAND:
            PlanEditorDialog::OnCommand(hDlg, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));

            return 0;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047bad0
int PlanEditorDialog::OnInit(HWND hDlg, HWND, LPARAM lParam)
{
    Win32::CenterWindow(hDlg, GetParent(hDlg));

    SetWindowLongA(hDlg, GWL_USERDATA, lParam);

    if (PlanEditorDialog::PopulateCameraList(hDlg) != 0)
    {
        ((PlanVM*)GetWindowLongA(hDlg, GWL_USERDATA))->LoadSymbolTables();

        PlanEditorDialog::PopulateBehaviorList(hDlg, g_pGameEngine->m_ppObjects[0]->GetSlot());

        return 1;
    }

    EndDialog(hDlg, -1);

    return 1;
}

// FUNCTION: HYPERX 0x0047bb50
int PlanEditorDialog::PopulateCameraList(HWND hDlg)
{
    int nResult = 0;

    const HWND hCombo = GetDlgItem(hDlg, IDC_PLAN_CAMERA);

    if (hCombo != 0)
    {
        for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
        {
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)g_pGameEngine->m_ppObjects[i]->m_pszName);
        }

        nResult = 1;

        SendMessageA(hCombo, CB_SETCURSEL, 0, 0);
    }

    return nResult;
}

// The list ends at the first empty name, which PlanName answers past the count.
// FUNCTION: HYPERX 0x0047bbc0
void PlanEditorDialog::PopulateBehaviorList(HWND hDlg, short nCamera)
{
    short i = 0;
    char* pszName;

    const HWND hCombo = GetDlgItem(hDlg, IDC_PLAN_BEHAVIOR);

    if (hCombo != 0)
    {
        SendMessageA(hCombo, CB_RESETCONTENT, 0, 0);

        while (*(pszName = PlanName(g_pGameEngine->m_ppSlots[nCamera], i++)) != 0)
        {
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)pszName);
        }

        pszName = g_pGameEngine->m_ppSlots[nCamera]->GetPlanState()->m_pszName;

        SendMessageA(hCombo, CB_SETCURSEL, g_pGameEngine->m_ppSlots[nCamera]->FindPlanByName(pszName), 0);

        PlanEditor::ApplyBehaviorSelection(hDlg, GetDlgItem(hDlg, IDC_PLAN_NODE));
    }
}

// FUNCTION: HYPERX 0x0047bc90
int PlanEditorDialog::OnCommand(HWND hDlg, int nId, HWND hwndCtl, int nCode)
{
    switch (nId)
    {
        case IDOK:
        case IDCANCEL:
            return PlanEditorDialog::OnClose(hDlg, nId, hwndCtl, nCode);

        case IDC_PLAN_LOAD:
            return PlanEditorDialog::OnLoadPlan(hDlg, nId, hwndCtl, nCode);

        case IDC_PLAN_CAMERA:
            return PlanEditorDialog::OnCameraChanged(hDlg, nId, hwndCtl, nCode);

        case IDC_PLAN_BEHAVIOR:
            return PlanEditorDialog::OnBehaviorChanged(hDlg, nId, hwndCtl, nCode);

        case IDC_PLAN_COMPILE:
            return PlanEditorDialog::OnCompilePlan(hDlg, nId, hwndCtl, nCode);

        case IDC_PLAN_NODE:
            return PlanEditorDialog::OnNodeChanged(hDlg, nId, hwndCtl, nCode);

        case IDC_PLAN_COMPILE_ALL:
            return PlanEditorDialog::OnCompileAll(hDlg, nId, hwndCtl, nCode);

        case IDC_PLAN_ACTIVATE:
            return PlanEditorDialog::OnActivateBehavior(hDlg, nId, hwndCtl, nCode);

        case IDC_PLAN_TRIGGER:
            return PlanEditorDialog::OnTriggerCondition(hDlg, nId, hwndCtl, nCode);
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047be10
int PlanEditorDialog::OnClose(HWND hDlg, int, HWND, int nCode)
{
    if (nCode == BN_CLICKED)
    {
        PlanVM* const pVM = (PlanVM*)GetWindowLongA(hDlg, GWL_USERDATA);

        delete pVM;

        EndDialog(hDlg, 1);

        return 0;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047be50
int PlanEditorDialog::OnCameraChanged(HWND hDlg, int, HWND, int nCode)
{
    if (nCode == CBN_SELCHANGE)
    {
        SendMessageA(GetDlgItem(hDlg, IDC_PLAN_NODE), CB_RESETCONTENT, 0, 0);
        SendMessageA(GetDlgItem(hDlg, IDC_PLAN_CONDITION), CB_RESETCONTENT, 0, 0);

        PlanEditorDialog::PopulateBehaviorList(hDlg, PlanEditorDialog::ApplyCameraSelection(hDlg));

        return 0;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047beb0
int PlanEditorDialog::OnBehaviorChanged(HWND hDlg, int, HWND, int nCode)
{
    if (nCode == CBN_SELCHANGE)
    {
        PlanEditor::ApplyBehaviorSelection(hDlg, GetDlgItem(hDlg, IDC_PLAN_NODE));

        return 0;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047bee0
int PlanEditorDialog::OnNodeChanged(HWND hDlg, int, HWND, int nCode)
{
    if (nCode == CBN_SELCHANGE)
    {
        PlanEditor::ApplyNodeSelection(hDlg, GetDlgItem(hDlg, IDC_PLAN_CONDITION));

        return 0;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047bf10
int PlanEditorDialog::OnLoadPlan(HWND hDlg, int, HWND, int nCode)
{
    char* pszName;
    void* pOut;

    if (nCode == BN_CLICKED)
    {
        const short nCamera   = PlanEditorDialog::ApplyCameraSelection(hDlg);
        const short nBehavior = (short)PlanEditorDialog::GetBehaviorSel(hDlg);

        pszName = PlanName(g_pGameEngine->m_ppSlots[nCamera], nBehavior);

        PathString strPath(pszName, MAX_PATH);

        g_pGameEngine->m_ppSlots[nCamera]->m_pPlanState =
            &g_pGameEngine->m_ppSlots[nCamera]->m_pPlanStates[nBehavior];

        PlanWord* const pBinary = PlanFile::OpenBinary(strPath.m_str.m_pchData, &pOut);

        if (pBinary != 0)
        {
            if (g_pGameEngine->m_ppSlots[nCamera]->GetPlanState()->SetAndInit(pBinary) == 0)
            {
                // STRING: HYPERX 0x004c69a4
                g_gameApp.Error("Cannot set and init this plan");

                delete[] pBinary;
            }
            else
            {
                delete[] pBinary;

                g_pGameEngine->m_ppSlots[nCamera]->InstantiateAIPlan(nBehavior);

                g_pGameEngine->m_ppSlots[nCamera]->GetPlanState()->ActivateBehavior(0);

                // STRING: HYPERX 0x004c6984
                g_gameApp.Error("New plan successfully loaded");
            }
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047c0a0
int PlanEditorDialog::OnCompilePlan(HWND hDlg, int, HWND, int nCode)
{
    if (nCode != 0)
    {
        return 0;
    }

    const short nCamera   = PlanEditorDialog::ApplyCameraSelection(hDlg);
    const short nBehavior = (short)PlanEditorDialog::GetBehaviorSel(hDlg);

    PlanVM* const pFile = PlanEditorDialog::LoadPlanText(hDlg, nBehavior, nCamera);

    try
    {
        if (pFile->Compile() != 0)
        {
            pFile->SaveBinary(PlanName(g_pGameEngine->m_ppSlots[nCamera], nBehavior));

            // STRING: HYPERX 0x004c69c4
            g_gameApp.Error("Compilation successful!");
        }
    }
    catch (AdlibError& err)
    {
        switch (err.nCode)
        {
            case 1:
                g_gameApp.Error(err.pszText);
                break;

            case 2:
                g_gameApp.Fatal(err.pszText);
                break;
        }
    }
    catch (char* pszText)
    {
        g_gameApp.Error(pszText);
    }

    return 0;
}

// The try sits inside both loops, so a plan that throws costs only its own name.

// FUNCTION: HYPERX 0x0047c1a0
int PlanEditorDialog::OnCompileAll(HWND hDlg, int, HWND, int nCode)
{
    char* pszName;

    if (nCode != 0)
    {
        return 0;
    }

    for (short i = 0; i < g_pGameEngine->m_nObjectCount; i++)
    {
        PlanVM* const pFile = (PlanVM*)GetWindowLongA(hDlg, GWL_USERDATA);

        for (short j = 0;; j++)
        {
            pszName = PlanName(g_pGameEngine->m_ppObjects[i], j);

            if (*pszName != 0)
            {
                pFile->LoadText(i, pszName);

                try
                {
                    pFile->Compile();

                    pFile->SaveBinary(pszName);
                }
                catch (AdlibError& err)
                {
                    switch (err.nCode)
                    {
                        case 1:
                            g_gameApp.Error(err.pszText);
                            break;

                        case 2:
                            g_gameApp.Fatal(err.pszText);
                            break;
                    }
                }
                catch (char* pszText)
                {
                    g_gameApp.Error(pszText);
                }
            }
            else
            {
                break;
            }
        }
    }

    // STRING: HYPERX 0x004c69c4
    g_gameApp.Error("Compilation successful!");

    return 0;
}

// A mismatch means the list shows a plan that has since been replaced.
// FUNCTION: HYPERX 0x0047c2f0
int PlanEditorDialog::OnActivateBehavior(HWND hDlg, int, HWND, int nCode)
{
    char szText[MAX_PATH];

    if (nCode != 0)
    {
        return 0;
    }

    const short nCamera = PlanEditorDialog::ApplyCameraSelection(hDlg);

    SendMessageA(GetDlgItem(hDlg, IDC_PLAN_BEHAVIOR), CB_GETLBTEXT, (short)PlanEditorDialog::GetBehaviorSel(hDlg), (LPARAM)szText);

    char* const pszName = g_pGameEngine->m_ppSlots[nCamera]->GetPlanState()->m_pszName;

    if (_mbscmp((const unsigned char*)pszName, (const unsigned char*)szText) != 0)
    {
        // STRING: HYPERX 0x004c69dc
        g_gameApp.Error("This is not the Plan you think it is.  Press the Load Plan button.");

        return 0;
    }

    g_pGameEngine->m_ppSlots[nCamera]->GetPlanState()->ActivateBehavior(
        (short)PlanEditorDialog::GetNodeSel(hDlg));

    return 0;
}

// FUNCTION: HYPERX 0x0047c3c0
int PlanEditorDialog::OnTriggerCondition(HWND hDlg, int, HWND, int nCode)
{
    char szNode[MAX_PATH];

    if (nCode == BN_CLICKED)
    {
        SendMessageA(GetDlgItem(hDlg, IDC_PLAN_NODE), CB_GETLBTEXT, (short)PlanEditorDialog::GetNodeSel(hDlg), (LPARAM)szNode);

        const HWND hCondList = GetDlgItem(hDlg, IDC_PLAN_CONDITION);

        if (hCondList != 0)
        {
            const short nCond = (short)PlanEditorDialog::GetCondSel(hDlg);

            char* const pszCond = new char[SendMessageA(hCondList, CB_GETLBTEXTLEN, nCond, 0) + 1];

            if (pszCond != 0)
            {
                const short nCamera = PlanEditorDialog::ApplyCameraSelection(hDlg);

                SendMessageA(hCondList, CB_GETLBTEXT, nCond, (LPARAM)pszCond);

                const GameBehaviorObject* const pCurrent = g_pGameEngine->m_ppSlots[nCamera]->GetPlanState()->m_pCurrent;

                if (_mbscmp((const unsigned char*)pCurrent->GetName(), (const unsigned char*)szNode) != 0)
                {
                    // STRING: HYPERX 0x004c6a20
                    g_gameApp.Error("This is not the Behavior you think.  Press the Load Plan and Proceed To buttons.");
                }
                else
                {
                    EngineObject* const pObject = g_pGameEngine->m_ppSlots[nCamera];

                    pObject->PostSceneEvent(
                        (int)((PlanVM*)GetWindowLongA(hDlg, GWL_USERDATA))
                            ->LookupSymbol(pszCond)
                            .nValue,
                        0);
                }

                delete[] pszCond;
            }
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047c4f0
PlanVM* __cdecl PlanEditorDialog::LoadPlanText(HWND hDlg, short nBehavior, short nCamera)
{
    PlanVM* const pFile = (PlanVM*)GetWindowLongA(hDlg, GWL_USERDATA);

    pFile->LoadText(nCamera, PlanName(g_pGameEngine->m_ppSlots[nCamera], nBehavior));

    return pFile;
}
