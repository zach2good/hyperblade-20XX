// Bring-up tracing, snapshots and the frame limiter. None of it is in the measured build.

#ifdef HYPERX_BRINGUP

#include "common/compat.h"
#include "engine/objectbase.h"
#include "app/gameapp.h"
#include "engine/devicector.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/messageids.h"
#include "physics/physicsbody.h"
#include "engine/cameraobject.h"
#include "globals.h"
#include "graphics/graphicsglobals.h"
#include "graphics/renderstate.h"
#include <ddraw.h>
#include "input/keyboarddeviceobj.h"
#include "ui/screen.h"
#include "ui/widget.h"
#include "graphics/gdi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/pooledclasses.h"
#include "planvm/planvmglobals.h"
#include "engine/game.h"
#include "graphics/d3ddeviceinit.h"
#include "ui/hudglobals.h"
#include "math/floatbits.h"
#include "planvm/decisionlog.h"

namespace BringUp
{

#define CRASH_STACK_WORDS  0x100
#define CRASH_STACK_FRAMES 8
#define CRASH_STACK_DUMP   24

// The log name comes from the image's file name.
#define BRINGUP_VARIANT_NONE 0
#define BRINGUP_VARIANT_SOFT 1

static void ImagePath(const char** ppszDirectory, const char** ppszStem)
{
    static char s_szDirectory[MAX_PATH];
    static char s_szStem[MAX_PATH];
    static int  s_bRead = 0;

    if (s_bRead == 0)
    {
        char         szPath[MAX_PATH];
        unsigned int nLength;
        char*        pszLastSeparator;
        char*        pszDot;

        s_bRead          = 1;
        s_szDirectory[0] = 0;
        strcpy(s_szStem, "bringup");

        nLength = GetModuleFileNameA(0, szPath, sizeof(szPath));

        if (nLength != 0 && nLength < sizeof(szPath))
        {
            pszLastSeparator = strrchr(szPath, '\\');

            if (pszLastSeparator != 0)
            {
                strcpy(s_szStem, pszLastSeparator + 1);
                *(pszLastSeparator + 1) = 0;
                strcpy(s_szDirectory, szPath);
            }
            else
            {
                strcpy(s_szStem, szPath);
            }

            pszDot = strrchr(s_szStem, '.');

            if (pszDot != 0)
            {
                *pszDot = 0;
            }
        }
    }

    if (ppszDirectory != 0)
    {
        *ppszDirectory = s_szDirectory;
    }

    if (ppszStem != 0)
    {
        *ppszStem = s_szStem;
    }
}

static int EndsWith(const char* pszText, const char* pszTail)
{
    const size_t nText = strlen(pszText);
    const size_t nTail = strlen(pszTail);

    if (nText < nTail)
    {
        return 0;
    }

    return _stricmp(pszText + nText - nTail, pszTail) == 0;
}

static int EnvInt(const char* pszName, int nDefault)
{
    const char* const pszValue = getenv(pszName);

    if (pszValue == 0)
    {
        return nDefault;
    }

    return atoi(pszValue);
}

static int Variant(void)
{
    static int s_nVariant = -1;

    if (s_nVariant == -1)
    {
        const char* pszStem = 0;

        ImagePath(0, &pszStem);

        s_nVariant = BRINGUP_VARIANT_NONE;

        if (EndsWith(pszStem, "_soft"))
        {
            s_nVariant = BRINGUP_VARIANT_SOFT;
        }
    }

    return s_nVariant;
}

static FILE* GetLog(void)
{
    static FILE* s_pLog     = 0;
    static int   s_bChecked = 0;

    if (!s_bChecked)
    {
        // Beside the image, named for it, so the three variants never share a log.
        char        szLog[MAX_PATH];
        const char* pszDirectory = 0;
        const char* pszStem      = 0;

        s_bChecked = 1;

        ImagePath(&pszDirectory, &pszStem);

        if (strlen(pszDirectory) + strlen(pszStem) + 5 < sizeof(szLog))
        {
            strcpy(szLog, pszDirectory);

            if (Variant() == BRINGUP_VARIANT_NONE)
            {
                strcat(szLog, "bringup.log");
            }
            else
            {
                strcat(szLog, pszStem);
                strcat(szLog, ".log");
            }
        }
        else
        {
            strcpy(szLog, "bringup.log");
        }

        s_pLog = fopen(szLog, "a");

        // The log is appended to, and bringup_run.read_log() splits runs on this banner.
        if (s_pLog != 0)
        {
            // The image relaunches itself, so a late child of an older run can write into this log.
            const char* pszTag = getenv("HB_RUN_TAG");

            fputs("bringupRunStart\n", s_pLog);

            if (pszTag != 0)
            {
                fprintf(s_pLog, "runTag = %s\n", pszTag);
            }

            fflush(s_pLog);
        }
    }

    return s_pLog;
}

// Writes the faulting address, raw and as an image offset, for tools/bringup_where.py.
static long WINAPI OnCrash(struct _EXCEPTION_POINTERS* pInfo)
{
    FILE* const pFile = GetLog();

    if (pFile != 0)
    {
        const unsigned int nAddress = (unsigned int)pInfo->ExceptionRecord->ExceptionAddress;

        fprintf(pFile, "CRASH code = 0x%08x at = 0x%08x rva = 0x%08x\n", (unsigned int)pInfo->ExceptionRecord->ExceptionCode, nAddress, nAddress - 0x400000);

        // ECX is usually the faulting call's this, and EAX the vtable it read.
        fprintf(pFile, "CRASH regs eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x"
                       " esp=%08x ebp=%08x\n",
                (unsigned int)pInfo->ContextRecord->Eax,
                (unsigned int)pInfo->ContextRecord->Ebx,
                (unsigned int)pInfo->ContextRecord->Ecx,
                (unsigned int)pInfo->ContextRecord->Edx,
                (unsigned int)pInfo->ContextRecord->Esi,
                (unsigned int)pInfo->ContextRecord->Edi,
                (unsigned int)pInfo->ContextRecord->Esp,
                (unsigned int)pInfo->ContextRecord->Ebp);

        if (pInfo->ExceptionRecord->NumberParameters >= 2)
        {
            fprintf(pFile, "CRASH operation = %u address = 0x%08x\n", (unsigned int)pInfo->ExceptionRecord->ExceptionInformation[0], (unsigned int)pInfo->ExceptionRecord->ExceptionInformation[1]);
        }

        {
            unsigned int* pAt    = (unsigned int*)pInfo->ContextRecord->Esp;
            int           nFound = 0;
            int           nStep;

            for (nStep = 0; nStep < CRASH_STACK_WORDS && nFound < CRASH_STACK_FRAMES; nStep++)
            {
                unsigned int nWord = pAt[nStep];

                if (nWord > 0x00401000 && nWord < 0x004b0000)
                {
                    // A first hit at +0 names the caller of a call through null.
                    fprintf(pFile, "CRASH from = 0x%08x at esp+0x%x\n", nWord, nStep * 4);

                    nFound++;
                }
            }
        }

        {
            unsigned int* const pAt = (unsigned int*)pInfo->ContextRecord->Esp;
            int                 nStep;

            for (nStep = 0; nStep < CRASH_STACK_DUMP; nStep++)
            {
                fprintf(pFile, "CRASH word esp+0x%02x = 0x%08x\n", nStep * 4, pAt[nStep]);
            }
        }

        fflush(pFile);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

static struct CrashHandler
{
    CrashHandler()
    {
        SetUnhandledExceptionFilter(OnCrash);
    }
} g_bringUpCrashHandler;

void Trace(const char* pszWhat, int nValue)
{
    FILE* const pFile = GetLog();

    if (pFile != 0)
    {
        fprintf(pFile, "%s = %d (0x%x)\n", pszWhat, nValue, (unsigned int)nValue);
        fflush(pFile);
    }
}

void TraceFloat(const char* pszWhat, float flValue)
{
    FILE* const pFile = GetLog();

    if (pFile != 0)
    {
        fprintf(pFile, "%s = %f\n", pszWhat, flValue);
        fflush(pFile);
    }
}

void TraceVec3(const char* pszWhat, float x, float y, float z)
{
    FILE* const pFile = GetLog();

    if (pFile != 0)
    {
        fprintf(pFile, "%s = (%f, %f, %f)\n", pszWhat, x, y, z);
        fflush(pFile);
    }
}

void TraceText(const char* pszWhat, const char* pszValue)
{
    FILE* const pFile = GetLog();

    if (pszValue == 0)
    {
        pszValue = "(none)";
    }

    if (pFile != 0)
    {
        fprintf(pFile, "%s = [%s]\n", pszWhat, pszValue);
        fflush(pFile);
    }
}

// Identical snapshots are skipped, since the splash redraws in a loop. HB_SNAP_MAX caps the rest.
void SaveBitmap(const char* pszWhat, void* hBitmap);

// A menu paints through GDI into the BeginPaint DC, so it is captured before EndPaint.
void SaveWindowDC(const char* pszWhat, void* hDeviceContext, int nWidth, int nHeight)
{
    const HDC hdc = (HDC)hDeviceContext;
    HDC       hdcCopy;
    HBITMAP   hbm;
    HGDIOBJ   hPrevious;

    if (hdc == 0)
    {
        return;
    }

    hdcCopy = CreateCompatibleDC(hdc);

    if (hdcCopy == 0)
    {
        return;
    }

    hbm = CreateCompatibleBitmap(hdc, nWidth, nHeight);

    if (hbm != 0)
    {
        hPrevious = SelectObject(hdcCopy, hbm);

        BitBlt(hdcCopy, 0, 0, nWidth, nHeight, hdc, 0, 0, SRCCOPY);

        SelectObject(hdcCopy, hPrevious);

        SaveBitmap(pszWhat, hbm);

        DeleteObject(hbm);
    }

    DeleteDC(hdcCopy);
}

void SaveBitmap(const char* pszWhat, void* hBitmap)
{
    static int          s_nNext      = 0;
    static int          s_nLimit     = -1;
    static int          s_nEvery     = 1;
    static int          s_nOnlyState = -1;
    static int          s_nOffered   = 0;
    static unsigned int s_nLastSum   = 0;

    unsigned int nSum = 0;
    long         nIndex;

    const HBITMAP    hbm = (HBITMAP)hBitmap;
    BITMAP           bm;
    BITMAPINFOHEADER info;
    BITMAPFILEHEADER header;
    unsigned char*   pPixels;
    long             nStride;
    long             nBytes;
    char             szName[64];
    HDC              hdc;
    FILE*            pFile;

    if (s_nLimit < 0)
    {
        s_nLimit     = EnvInt("HB_SNAP_MAX", 16);
        s_nEvery     = EnvInt("HB_SNAP_EVERY", 1);
        s_nOnlyState = EnvInt("HB_SNAP_STATE", -1);

        if (s_nEvery < 1)
        {
            s_nEvery = 1;
        }

        Trace("snapLimit", s_nLimit);
        Trace("snapEvery", s_nEvery);
        Trace("snapState", s_nOnlyState);
    }

    if (s_nNext >= s_nLimit)
    {
        return;
    }

    if (s_nOnlyState >= 0 && (int)g_gameApp.m_nEngineState != s_nOnlyState)
    {
        return;
    }

    s_nOffered++;

    if (s_nOffered % s_nEvery != 0)
    {
        return;
    }

    if (hbm == 0 || GetObjectA(hbm, sizeof(bm), &bm) == 0)
    {
        Trace("snapNoBitmap", (int)hBitmap);

        return;
    }

    nStride = ((bm.bmWidth * 3 + 3) / 4) * 4;
    nBytes  = nStride * bm.bmHeight;

    pPixels = (unsigned char*)malloc(nBytes);

    if (pPixels == 0)
    {
        Trace("snapNoMemory", (int)nBytes);

        return;
    }

    memset(&info, 0, sizeof(info));
    info.biSize        = sizeof(info);
    info.biWidth       = bm.bmWidth;
    info.biHeight      = bm.bmHeight;
    info.biPlanes      = 1;
    info.biBitCount    = 24;
    info.biCompression = BI_RGB;

    hdc = GetDC(0);

    if (hdc == 0 || GetDIBits(hdc, hbm, 0, bm.bmHeight, pPixels, (BITMAPINFO*)&info, DIB_RGB_COLORS) == 0)
    {
        Trace("snapNoBits", (int)GetLastError());

        if (hdc != 0)
        {
            ReleaseDC(0, hdc);
        }

        free(pPixels);

        return;
    }

    ReleaseDC(0, hdc);

    for (nIndex = 0; nIndex < nBytes; nIndex++)
    {
        nSum = nSum * 33 + pPixels[nIndex];
    }

    {
        static int s_nSaidSum = 0;

        if (s_nSaidSum < 4)
        {
            s_nSaidSum++;

            Trace("snapSum", (int)nSum);
            TraceText("snapSumFor", pszWhat);
        }
    }

    if (nSum == s_nLastSum)
    {
        free(pPixels);

        return;
    }

    s_nLastSum = nSum;

    sprintf(szName, "snap_%02d_%s.bmp", s_nNext, pszWhat);
    s_nNext++;

    pFile = fopen(szName, "wb");

    if (pFile == 0)
    {
        Trace("snapNoFile", 0);

        free(pPixels);

        return;
    }

    memset(&header, 0, sizeof(header));
    header.bfType    = 0x4d42;
    header.bfOffBits = sizeof(header) + sizeof(info);
    header.bfSize    = header.bfOffBits + nBytes;

    fwrite(&header, sizeof(header), 1, pFile);
    fwrite(&info, sizeof(info), 1, pFile);
    fwrite(pPixels, nBytes, 1, pFile);
    fclose(pFile);

    free(pPixels);

    TraceText("snapWrote", szName);
}

// HB_FIXTURE=home,away,arena. State three reseeds all three every frame, so this runs last.
void PickFixture(void)
{
    static int s_bRead = 0;
    static int s_anPick[3];
    static int s_nPicked = 0;

    if (s_bRead == 0)
    {
        const char* pszPick = getenv("HB_FIXTURE");

        s_bRead = 1;

        if (pszPick != 0)
        {
            while (*pszPick != 0 && s_nPicked < 3)
            {
                s_anPick[s_nPicked] = atoi(pszPick);
                s_nPicked++;

                while (*pszPick != 0 && *pszPick != ',')
                {
                    pszPick++;
                }

                if (*pszPick == ',')
                {
                    pszPick++;
                }
            }
        }
    }

    if (s_nPicked < 3)
    {
        return;
    }

    g_nDromeTeamHome = s_anPick[0];
    g_nDromeTeamAway = s_anPick[1];
    g_nDromeArena    = s_anPick[2];

    Trace("pickHome", g_nDromeTeamHome);
    Trace("pickAway", g_nDromeTeamAway);
    Trace("pickArena", g_nDromeArena);
}

static int KeyCode(const char* pszName, int nLength)
{
    if (nLength == 1)
    {
        char ch = pszName[0];

        if (ch >= 'a' && ch <= 'z')
        {
            return ch - 'a' + 'A';
        }

        return ch;
    }

    if (nLength == 2 && pszName[0] == 'u' && pszName[1] == 'p')
    {
        return VK_UP;
    }

    if (nLength == 4 && pszName[0] == 'd')
    {
        return VK_DOWN;
    }

    if (nLength == 4 && pszName[0] == 'l')
    {
        return VK_LEFT;
    }

    if (nLength == 5 && pszName[0] == 'r')
    {
        return VK_RIGHT;
    }

    if (nLength == 5 && pszName[0] == 's')
    {
        return VK_SPACE;
    }

    if (nLength == 5 && pszName[0] == 'e')
    {
        return VK_RETURN;
    }

    return 0;
}

// HB_HOLD_KEYS=up,left holds keys through the match, as a real WM_KEYDOWN would.
void HoldKeys(void)
{
    static int s_bRead = 0;
    static int s_anKeys[8];
    static int s_nKeys     = 0;
    static int s_bSaid     = 0;
    static int s_nReported = 0;

    int nKey;

    if (s_bRead == 0)
    {
        const char* pszKeys = getenv("HB_HOLD_KEYS");

        s_bRead = 1;

        while (pszKeys != 0 && *pszKeys != 0 && s_nKeys < 8)
        {
            const char* pszStart = pszKeys;

            while (*pszKeys != 0 && *pszKeys != ',')
            {
                pszKeys++;
            }

            nKey = KeyCode(pszStart, (int)(pszKeys - pszStart));

            if (nKey != 0)
            {
                s_anKeys[s_nKeys] = nKey;
                s_nKeys++;
            }

            if (*pszKeys == ',')
            {
                pszKeys++;
            }
        }
    }

    if (s_nKeys == 0 || g_pInputDevice == 0 || g_pGameEngine == 0)
    {
        return;
    }

    if (g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
    {
        return;
    }

    if ((short)g_pGameEngine->m_wRunState != ENGINE_RUN_PLAYING)
    {
        return;
    }

    if (s_bSaid == 0)
    {
        s_bSaid = 1;
        Trace("holdKeysCount", s_nKeys);
    }

    if (s_nReported < 6)
    {
        Trace("holdMaskBefore", (int)g_pInputDevice->m_nHeldMask);
    }

    // HandleKeyDown drops a second press with no release between, so release first.
    for (nKey = 0; nKey < s_nKeys; nKey++)
    {
        g_pInputDevice->HandleKeyUp((unsigned int)s_anKeys[nKey]);
        g_pInputDevice->HandleKeyDown((unsigned int)s_anKeys[nKey]);
    }

    if (s_nReported < 6)
    {
        s_nReported++;
        Trace("holdMask", (int)g_pInputDevice->m_nHeldMask);
    }
}

// HB_INPUT_ACTIONS=26,27 HB_INPUT_EVERY=300 posts actions no key reaches.
#define INPUT_ACTION_MAX 8

void SendInputActions(void)
{
    static int s_bRead = 0;
    static int s_anActions[INPUT_ACTION_MAX];
    static int s_nActions = 0;
    static int s_nEvery   = 120;
    static int s_nFrame   = 0;
    static int s_nAt      = 0;

    EngineObject* pEntity;
    int           nAction;

    if (s_bRead == 0)
    {
        const char* pszActions = getenv("HB_INPUT_ACTIONS");
        const char* pszEvery   = getenv("HB_INPUT_EVERY");

        s_bRead = 1;

        if (pszEvery != 0 && atoi(pszEvery) > 0)
        {
            s_nEvery = atoi(pszEvery);
        }

        while (pszActions != 0 && *pszActions != 0 && s_nActions < INPUT_ACTION_MAX)
        {
            s_anActions[s_nActions] = atoi(pszActions);
            s_nActions++;

            while (*pszActions != 0 && *pszActions != ',')
            {
                pszActions++;
            }

            if (*pszActions == ',')
            {
                pszActions++;
            }
        }

        if (s_nActions != 0)
        {
            Trace("inputActionCount", s_nActions);
            Trace("inputActionEvery", s_nEvery);
        }
    }

    if (s_nActions == 0 || g_pInputDevice == 0 || g_pGameEngine == 0)
    {
        return;
    }

    if (g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
    {
        return;
    }

    if ((short)g_pGameEngine->m_wRunState != ENGINE_RUN_PLAYING)
    {
        return;
    }

    pEntity = g_pInputDevice->m_pEntity;

    if (pEntity == 0)
    {
        return;
    }

    s_nFrame++;

    if (s_nFrame < s_nEvery)
    {
        return;
    }

    s_nFrame = 0;
    nAction  = s_anActions[s_nAt];
    s_nAt    = (s_nAt + 1) % s_nActions;

    Trace("inputActionSent", nAction);

    pEntity->DispatchSceneMessage(MSG_NEW_INPUT, nAction, DECISION_LOG_FRESH, (unsigned int)pEntity->m_nSlot, 0);
}

// HB_CLICKS=x,y@frame;... sends a real click through the window procedure.
#define CLICK_MAX  16
#define CLICK_DOWN 0
#define CLICK_UP   1

void SendClicks(void* hWindow)
{
    static int s_bRead = 0;
    static int s_anX[CLICK_MAX];
    static int s_anY[CLICK_MAX];
    static int s_anFrame[CLICK_MAX];
    static int s_nClicks = 0;
    static int s_nFrame  = 0;
    static int s_nAt     = 0;
    static int s_nPhase  = CLICK_DOWN;

    LPARAM lPoint;

    if (s_bRead == 0)
    {
        const char* pszClicks = getenv("HB_CLICKS");

        s_bRead = 1;

        while (pszClicks != 0 && *pszClicks != 0 && s_nClicks < CLICK_MAX)
        {
            int nX     = 0;
            int nY     = 0;
            int nFrame = 0;

            if (sscanf(pszClicks, "%d,%d@%d", &nX, &nY, &nFrame) != 3)
            {
                break;
            }

            s_anX[s_nClicks]     = nX;
            s_anY[s_nClicks]     = nY;
            s_anFrame[s_nClicks] = nFrame;
            s_nClicks++;

            while (*pszClicks != 0 && *pszClicks != ';')
            {
                pszClicks++;
            }

            if (*pszClicks == ';')
            {
                pszClicks++;
            }
        }

        if (s_nClicks != 0)
        {
            Trace("clickCount", s_nClicks);
        }
    }

    if (s_nAt >= s_nClicks || hWindow == 0)
    {
        return;
    }

    s_nFrame++;

    lPoint = MAKELPARAM(s_anX[s_nAt], s_anY[s_nAt]);

    if (s_nPhase == CLICK_DOWN)
    {
        if (s_nFrame < s_anFrame[s_nAt])
        {
            return;
        }

        Trace("clickDownX", s_anX[s_nAt]);
        Trace("clickDownY", s_anY[s_nAt]);
        Trace("clickDownState", (int)g_gameApp.m_nEngineState);

        SendMessageA((HWND)hWindow, WM_MOUSEMOVE, 0, lPoint);
        SendMessageA((HWND)hWindow, WM_LBUTTONDOWN, MK_LBUTTON, lPoint);

        s_nPhase = CLICK_UP;

        return;
    }

    SendMessageA((HWND)hWindow, WM_LBUTTONUP, 0, lPoint);

    Trace("clickUpState", (int)g_gameApp.m_nEngineState);

    s_nPhase = CLICK_DOWN;
    s_nAt++;
}

// HB_WIDGETS=id@frame[:msg];... hands the active screen a widget message by id.
#define WIDGET_MAX         16
#define WIDGET_DEFAULT_MSG 0x202

void SendWidgets(void)
{
    static int s_bRead = 0;
    static int s_anId[WIDGET_MAX];
    static int s_anFrame[WIDGET_MAX];
    static int s_anMsg[WIDGET_MAX];
    static int s_nWidgets = 0;
    static int s_nFrame   = 0;
    static int s_nAt      = 0;

    if (s_bRead == 0)
    {
        const char* pszWidgets = getenv("HB_WIDGETS");

        s_bRead = 1;

        while (pszWidgets != 0 && *pszWidgets != 0 && s_nWidgets < WIDGET_MAX)
        {
            int         nId    = 0;
            int         nFrame = 0;
            int         nMsg   = WIDGET_DEFAULT_MSG;
            const char* pszColon;

            if (sscanf(pszWidgets, "%d@%d", &nId, &nFrame) != 2)
            {
                break;
            }

            pszColon = pszWidgets;

            while (*pszColon != 0 && *pszColon != ';' && *pszColon != ':')
            {
                pszColon++;
            }

            if (*pszColon == ':')
            {
                nMsg = (int)strtoul(pszColon + 1, 0, 0);
            }

            s_anId[s_nWidgets]    = nId;
            s_anFrame[s_nWidgets] = nFrame;
            s_anMsg[s_nWidgets]   = nMsg;
            s_nWidgets++;

            while (*pszWidgets != 0 && *pszWidgets != ';')
            {
                pszWidgets++;
            }

            if (*pszWidgets == ';')
            {
                pszWidgets++;
            }
        }

        if (s_nWidgets != 0)
        {
            Trace("widgetCount", s_nWidgets);
        }
    }

    if (s_nAt >= s_nWidgets)
    {
        return;
    }

    s_nFrame++;

    if (s_nFrame < s_anFrame[s_nAt] || g_pActiveScreen == 0)
    {
        return;
    }

    Trace("widgetSendId", s_anId[s_nAt]);
    Trace("widgetSendState", (int)g_gameApp.m_nEngineState);
    Trace("widgetSendMsg", s_anMsg[s_nAt]);

    g_pActiveScreen->OnWidgetMessage(s_anMsg[s_nAt], (unsigned int)s_anId[s_nAt]);

    s_nAt++;
}

// HB_DUMP_WIDGETS=frame. A slot with frames 0 was built and never given a bitmap.
void DumpWidgets(void)
{
    static int s_nAt    = -2;
    static int s_nFrame = 0;

    Widget** ppSlots;
    Widget*  pWidget;
    int      nSlot;

    if (s_nAt == -2)
    {
        s_nAt = EnvInt("HB_DUMP_WIDGETS", -1);
    }

    if (s_nAt < 0)
    {
        return;
    }

    s_nFrame++;

    if (s_nFrame < s_nAt || g_pActiveScreen == 0)
    {
        return;
    }

    s_nAt = -1;

    ppSlots = g_pActiveScreen->m_apWidgets;

    Trace("dumpWidgetsState", (int)g_gameApp.m_nEngineState);

    for (nSlot = 0; nSlot < 256; nSlot++)
    {
        pWidget = ppSlots[nSlot];

        if (pWidget == 0)
        {
            continue;
        }

        Trace("wSlot", nSlot);
        Trace("wX", pWidget->m_nX);
        Trace("wY", pWidget->m_nY);
        Trace("wHidden", pWidget->m_bHidden);
        Trace("wFrames", pWidget->m_nFrameIndex);
        Trace("wBitmap", pWidget->m_pFrames != 0 && pWidget->m_pFrames[0].hBitmap != 0);

        if (pWidget->m_pszText != 0)
        {
            TraceText("wText", pWidget->m_pszText);
        }
        else
        {
            TraceText("wText", "");
        }
    }
}

// HB_FATAL_STOPS=1 exits at the first fatal the bring-up build skips.
void OnFatalSkipped(void)
{
    static int s_nSeen  = 0;
    static int s_nStops = -1;

    if (s_nStops == -1)
    {
        s_nStops = EnvInt("HB_FATAL_STOPS", 0);
    }

    s_nSeen++;

    Trace("FATAL_SKIPPED_run_is_now_undefined", s_nSeen);

    if (s_nStops != 0)
    {
        Trace("fatalStopsRun", 1);

        // Not PostQuitMessage: the fatal's caller would run on before the pump saw it.
        exit(2);
    }
}

// HB_ROUTE=prev,playerMode,arena[,difficulty], for state sixteen. -2 leaves a word alone.
#define ROUTE_KEEP (-2)

void SetRoute(void)
{
    static int s_bRead = 0;
    static int s_anRoute[4];
    static int s_nRead = 0;

    if (s_bRead == 0)
    {
        const char* pszRoute = getenv("HB_ROUTE");

        s_bRead = 1;

        while (pszRoute != 0 && *pszRoute != 0 && s_nRead < 4)
        {
            s_anRoute[s_nRead] = atoi(pszRoute);
            s_nRead++;

            while (*pszRoute != 0 && *pszRoute != ',')
            {
                pszRoute++;
            }

            if (*pszRoute == ',')
            {
                pszRoute++;
            }
        }
    }

    if (s_nRead < 3)
    {
        return;
    }

    if (s_anRoute[0] != ROUTE_KEEP)
    {
        g_nPrevEngineState = (unsigned int)s_anRoute[0];
    }

    if (s_anRoute[1] != ROUTE_KEEP)
    {
        g_nPlayerMode = s_anRoute[1];
    }

    if (s_anRoute[2] != ROUTE_KEEP)
    {
        g_nDromeArena = s_anRoute[2];
    }

    // Through the setter, which copies each player's stat table and writes the ini.
    if (s_nRead > 3 && s_anRoute[3] != ROUTE_KEEP && g_pGameEngine != 0)
    {
        Trace("routeDifficultyWas", g_pGameEngine->m_nDifficulty);

        if (g_pGameEngine->m_pTeamA != 0 && g_pGameEngine->m_pTeamB != 0)
        {
            Trace("routeDifficultySet",
                  (int)g_pGameEngine->SetDifficultyLevel((unsigned int)s_anRoute[3]));
        }
        else
        {
            Trace("routeDifficultyEarly", s_anRoute[3]);
            g_pGameEngine->m_nDifficulty = s_anRoute[3];
        }
    }

    Trace("routePrev", (int)g_nPrevEngineState);
    Trace("routeMode", g_nPlayerMode);
    Trace("routeArena", g_nDromeArena);

    if (g_pGameEngine != 0)
    {
        Trace("routeDifficulty", g_pGameEngine->m_nDifficulty);
    }
}

#define WALK_MAX 16

// HB_LOOP_MATCH=1 sends the engine back to ESTATE_MATCH_START when it reaches the jumbotron.
void LoopMatch(int nWalkDone)
{
    static int s_bRead     = 0;
    static int s_bOn       = 0;
    static int s_nRestarts = 0;

    if (s_bRead == 0)
    {
        const char* pszLoop = getenv("HB_LOOP_MATCH");

        s_bRead = 1;

        if (pszLoop != 0)
        {
            s_bOn = atoi(pszLoop);
        }
    }

    if (s_bOn == 0 || nWalkDone == 0)
    {
        return;
    }

    if (g_gameApp.m_nEngineState != ESTATE_JUMBOTRON)
    {
        return;
    }

    s_nRestarts++;
    Trace("loopMatchRestart", s_nRestarts);
    SetRoute();
    Game::SetEngineState(ESTATE_MATCH_START);
}

// HB_WALK_STATES=3,4,16,14 HB_WALK_FRAMES=90 walks the states a person clicks through.
void WalkStates(void)
{
    static int s_bRead = 0;
    static int s_anPath[WALK_MAX];
    static int s_nPath   = 0;
    static int s_nAt     = 0;
    static int s_nFrames = 0;
    static int s_nSeen   = 0;

    if (s_bRead == 0)
    {
        const char* pszPath = getenv("HB_WALK_STATES");

        s_bRead   = 1;
        s_nFrames = EnvInt("HB_WALK_FRAMES", 90);

        if (pszPath != 0)
        {
            while (*pszPath != 0 && s_nPath < WALK_MAX)
            {
                s_anPath[s_nPath] = atoi(pszPath);
                s_nPath++;

                while (*pszPath != 0 && *pszPath != ',')
                {
                    pszPath++;
                }

                if (*pszPath == ',')
                {
                    pszPath++;
                }
            }
        }

        if (s_nPath > 0)
        {
            Trace("walkStates", s_nPath);
            Trace("walkTo", s_anPath[0]);
            Game::SetEngineState((unsigned int)s_anPath[0]);
        }
    }

    if (s_nPath == 0 || s_nAt + 1 >= s_nPath)
    {
        LoopMatch(1);

        return;
    }

    s_nSeen++;

    if (s_nSeen < s_nFrames)
    {
        return;
    }

    s_nSeen = 0;
    s_nAt++;

    Trace("walkTo", s_anPath[s_nAt]);
    SetRoute();
    Game::SetEngineState((unsigned int)s_anPath[s_nAt]);
}

// HB_PERIOD_MS sets the period length through the engine's own field; the ini is not touched.
void ShortenPeriod(void)
{
    static int s_nMs = -1;

    if (s_nMs == -1)
    {
        s_nMs = EnvInt("HB_PERIOD_MS", 0);
    }

    if (s_nMs <= 0 || g_pGameEngine == 0)
    {
        return;
    }

    if (g_pGameEngine->m_nPeriodLengthMs != s_nMs)
    {
        g_pGameEngine->m_nPeriodLengthMs = s_nMs;

        Trace("periodLengthMs", s_nMs);
    }
}

void DumpPlanMessages(void);

void ForceScores(int* pnHome, int* pnAway)
{
    static int s_bRead = 0;
    static int s_bOn   = 0;
    static int s_nHome = 0;
    static int s_nAway = 0;

    if (s_bRead == 0)
    {
        const char* pszScores = getenv("HB_SCORES");

        s_bRead = 1;

        if (pszScores != 0 && sscanf(pszScores, "%d,%d", &s_nHome, &s_nAway) == 2)
        {
            s_bOn = 1;

            Trace("forceScoreTeam1", s_nHome);
            Trace("forceScoreTeam2", s_nAway);
        }
    }

    if (s_bOn == 0)
    {
        return;
    }

    *pnHome = s_nHome;
    *pnAway = s_nAway;
}

// HB_QUIT_AFTER=N. The quit is posted, since clearing the active flag leaves the pump waiting.
void QuitWhenBudgetSpent(void)
{
    static int s_bRead   = 0;
    static int s_nBudget = 0;
    static int s_nSeen   = 0;

    if (s_bRead == 0)
    {
        s_bRead   = 1;
        s_nBudget = EnvInt("HB_QUIT_AFTER", 0);

        Trace("frameBudget", s_nBudget);
    }

    if (s_nBudget <= 0)
    {
        return;
    }

    s_nSeen++;

    if (s_nSeen < s_nBudget)
    {
        return;
    }

    Trace("frameBudgetSpent", s_nSeen);
    DumpPlanMessages();

    PostQuitMessage(0);
}

// Every plan message, counted. The slots cover every id up to MSG_NOMSG.
#define PLAN_MSG_SLOTS 160

static int s_anPlanMsgCount[PLAN_MSG_SLOTS];

void CountPlanMessage(int nEvent)
{
    if (nEvent < 0 || nEvent >= PLAN_MSG_SLOTS)
    {
        return;
    }

    if (s_anPlanMsgCount[nEvent] == 0)
    {
        Trace("planMsgFirst", nEvent);
    }

    s_anPlanMsgCount[nEvent]++;
}

void DumpPlanMessages(void)
{
    // Reached from the frame budget and from WM_DESTROY; whichever comes first prints.
    static int s_bDumped = 0;

    if (s_bDumped != 0)
    {
        return;
    }

    s_bDumped = 1;

    char szName[32];
    int  nEvent;
    int  nKinds;

    nKinds = 0;

    for (nEvent = 0; nEvent < PLAN_MSG_SLOTS; nEvent++)
    {
        if (s_anPlanMsgCount[nEvent] != 0)
        {
            nKinds++;
            sprintf(szName, "planMsgCount%d", nEvent);
            Trace(szName, s_anPlanMsgCount[nEvent]);
        }
    }

    Trace("planMsgKinds", nKinds);
}

#define CHANGE_SLOTS 32

void TraceChange(const char* pszWhat, int nValue)
{
    static const char* s_apszName[CHANGE_SLOTS];
    static int         s_anLast[CHANGE_SLOTS];
    static int         s_nUsed = 0;

    int nSlot;

    for (nSlot = 0; nSlot < s_nUsed; nSlot++)
    {
        if (strcmp(s_apszName[nSlot], pszWhat) == 0)
        {
            if (s_anLast[nSlot] == nValue)
            {
                return;
            }

            s_anLast[nSlot] = nValue;
            Trace(pszWhat, nValue);

            return;
        }
    }

    if (s_nUsed >= CHANGE_SLOTS)
    {
        Trace(pszWhat, nValue);

        return;
    }

    s_apszName[s_nUsed] = pszWhat;
    s_anLast[s_nUsed]   = nValue;
    s_nUsed++;

    Trace(pszWhat, nValue);
}

// HB_START_STATE writes what dismissing the start-up dialog writes, which a tooling run cannot do.
void ForceStartState(void)
{
    const char* pszState = getenv("HB_START_STATE");

    if (pszState == 0)
    {
        return;
    }

    g_nPrevEngineState       = 0xf;
    g_gameApp.m_nEngineState = (unsigned int)atoi(pszState);

    Trace("startState", (int)g_gameApp.m_nEngineState);
}

// Holds each splash frame, pumping messages so WM_PAINT is serviced.
#define HOLD_STAGES  3
#define HOLD_TICK_MS 50

void HoldSplash(void* hWindow)
{
    static const char* const apszFrames[HOLD_STAGES] = { "SPLASHWZ", "SPLASHAC", "HBLOGO" };

    const HWND hWnd    = (HWND)hWindow;
    const int  nHoldMs = EnvInt("HB_HOLD_MS", 1500);

    int nStage;
    int nTick;

    Trace("holdMs", nHoldMs);

    if (nHoldMs <= 0)
    {
        return;
    }

    for (nStage = 0; nStage < HOLD_STAGES; nStage++)
    {
        // Frames one and two need a timer id set; the third needs it clear.
        g_nSplashTimerId = (nStage + 1 < HOLD_STAGES);
        g_nSplashFrame   = nStage;
        g_bSplashBusy    = 0;

        GameApp::SplashTimerCallback(0, 0, (DWORD)hWnd, 0, 0);

        for (nTick = 0; nTick < nHoldMs / HOLD_TICK_MS; nTick++)
        {
            int bRepaint = 0;
            MSG msg;

            while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) != 0)
            {
                if (msg.message == WM_PAINT || msg.message == WM_ERASEBKGND)
                {
                    bRepaint = 1;
                }

                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }

            if (bRepaint != 0)
            {
                GDI::DrawBitmapToWindow(hWnd, apszFrames[nStage]);
            }

            Sleep(HOLD_TICK_MS);
        }
    }

    g_nSplashTimerId = 0;
}

// The deadline advances 33, 33 then 34 ms from itself; an overrun past a second restarts it.
#define BRINGUP_FRAME_MICROS 33333
#define BRINGUP_RESYNC_MS    1000

int InSizeMove(void);

int Now(void)
{
    return (int)timeGetTime();
}

void PaceFrame(void)
{
    static int          s_bStarted   = 0;
    static DWORD        s_dwDeadline = 0;
    static unsigned int s_nCarry     = 0;

    const DWORD dwNow = timeGetTime();
    DWORD       dwStep;
    long        lWait;

    if (s_bStarted == 0)
    {
        // This also refines the engine's clock. HB_COARSE_TIMER=1 leaves the period alone.
        if (getenv("HB_COARSE_TIMER") == 0)
        {
            timeBeginPeriod(1);
        }
        s_bStarted   = 1;
        s_dwDeadline = dwNow;
        s_nCarry     = 0;
    }

    if (InSizeMove() != 0)
    {
        s_dwDeadline = dwNow;

        return;
    }

    s_nCarry += BRINGUP_FRAME_MICROS;
    dwStep = s_nCarry / 1000;
    s_nCarry -= dwStep * 1000;
    s_dwDeadline += dwStep;

    lWait = (long)(s_dwDeadline - dwNow);

    if (lWait < -BRINGUP_RESYNC_MS)
    {
        s_dwDeadline = dwNow;

        return;
    }

    while (lWait > 0)
    {
        Sleep((DWORD)lWait);
        lWait = (long)(s_dwDeadline - timeGetTime());
    }
}

// Nothing gives a tooling run the focus, and MainUpdate does nothing without the active flag.
void HoldActive(void)
{
    static int s_bHeld = 0;
    static int s_bSeen = 0;

    if (s_bSeen == 0)
    {
        s_bSeen = 1;

        Trace("mainUpdateReached", 1);
    }

    if (g_gameApp.m_bIsActive != 0)
    {
        return;
    }

    if (s_bHeld == 0)
    {
        s_bHeld = 1;

        Trace("heldActive", 1);
    }

    g_gameApp.m_bIsActive = 1;
}

// HoldActive pins m_bIsActive, so the focus is tracked here for WndProc::OnActivate.
static int s_bBringUpFocused = 0;

int FocusChanged(unsigned int bActive)
{
    const int bFocused = bActive != 0;

    if (bFocused == s_bBringUpFocused)
    {
        return 0;
    }

    s_bBringUpFocused = bFocused;

    Trace("focus", bFocused);

    return 1;
}

// A key down when the focus leaves never sends WM_KEYUP here.
static unsigned char s_abKeyHeld[256];

void NoteKey(unsigned int nMessage, unsigned int nKey)
{
    if (nKey >= 256)
    {
        return;
    }

    if (nMessage == WM_KEYDOWN)
    {
        s_abKeyHeld[nKey] = 1;
    }
    else if (nMessage == WM_KEYUP)
    {
        s_abKeyHeld[nKey] = 0;
    }
}

void ReleaseHeldKeys(void)
{
    int nKey;
    int nReleased = 0;

    for (nKey = 0; nKey < 256; nKey++)
    {
        if (s_abKeyHeld[nKey] == 0)
        {
            continue;
        }

        s_abKeyHeld[nKey] = 0;
        nReleased++;

        if (g_pInputDevice != 0)
        {
            g_pInputDevice->HandleKeyUp((unsigned int)nKey);
        }
    }

    if (nReleased != 0)
    {
        Trace("releasedHeldKeys", nReleased);
    }
}

// Stretches the 640x480 back buffer to the client area, letterboxed in black.
void PresentFrame(void* hWindowDC, void* hBackDC, int cx, int cy)
{
    const HDC hdcWindow = (HDC)hWindowDC;
    const HDC hdcBack   = (HDC)hBackDC;
    RECT      rcClient;
    RECT      rcBar;
    int       cxClient;
    int       cyClient;
    int       cxDest;
    int       cyDest;
    int       x;
    int       y;

    GetClientRect(g_gameApp.m_hWnd, &rcClient);

    cxClient = rcClient.right;
    cyClient = rcClient.bottom;

    if (cxClient <= 0 || cyClient <= 0 || cx <= 0 || cy <= 0)
    {
        return;
    }

    if (cxClient == cx && cyClient == cy)
    {
        BitBlt(hdcWindow, 0, 0, cx, cy, hdcBack, 0, 0, SRCCOPY);

        return;
    }

    cxDest = cxClient;
    cyDest = cxClient * cy / cx;

    if (cyDest > cyClient)
    {
        cyDest = cyClient;
        cxDest = cyClient * cx / cy;
    }

    x = (cxClient - cxDest) / 2;
    y = (cyClient - cyDest) / 2;

    SetStretchBltMode(hdcWindow, COLORONCOLOR);
    StretchBlt(hdcWindow, x, y, cxDest, cyDest, hdcBack, 0, 0, cx, cy, SRCCOPY);

    if (x > 0)
    {
        SetRect(&rcBar, 0, 0, x, cyClient);
        FillRect(hdcWindow, &rcBar, (HBRUSH)GetStockObject(BLACK_BRUSH));
        SetRect(&rcBar, x + cxDest, 0, cxClient, cyClient);
        FillRect(hdcWindow, &rcBar, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    if (y > 0)
    {
        SetRect(&rcBar, 0, 0, cxClient, y);
        FillRect(hdcWindow, &rcBar, (HBRUSH)GetStockObject(BLACK_BRUSH));
        SetRect(&rcBar, 0, y + cyDest, cxClient, cyClient);
        FillRect(hdcWindow, &rcBar, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }
}

// Menus paint and hit-test at the origin unscaled, so only the surround is cleared.
void ClearSurround(void* hWindow, int cx, int cy)
{
    const HWND hWnd = (HWND)hWindow;
    RECT       rcClient;
    RECT       rcBar;
    HDC        hdc;

    GetClientRect(hWnd, &rcClient);

    if (rcClient.right <= cx && rcClient.bottom <= cy)
    {
        return;
    }

    hdc = GetDC(hWnd);

    if (hdc == 0)
    {
        return;
    }

    if (rcClient.right > cx)
    {
        SetRect(&rcBar, cx, 0, rcClient.right, rcClient.bottom);
        FillRect(hdc, &rcBar, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    if (rcClient.bottom > cy)
    {
        SetRect(&rcBar, 0, cy, rcClient.right, rcClient.bottom);
        FillRect(hdc, &rcBar, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    ReleaseDC(hWnd, hdc);
}

// A drag runs a modal loop inside DispatchMessageA; the timer keeps frames coming.
#define SIZEMOVE_TIMER_ID 0x4252
#define SIZEMOVE_TIMER_MS 33

static int s_bInSizeMove    = 0;
static int s_bInTimerUpdate = 0;

void OnEnterSizeMove(void* hWindow)
{
    const HWND hWnd = (HWND)hWindow;

    s_bInSizeMove = 1;

    SetTimer(hWnd, SIZEMOVE_TIMER_ID, SIZEMOVE_TIMER_MS, 0);

    Trace("enterSizeMove", 1);
}

void OnExitSizeMove(void* hWindow)
{
    const HWND hWnd = (HWND)hWindow;

    KillTimer(hWnd, SIZEMOVE_TIMER_ID);

    s_bInSizeMove = 0;

    Trace("exitSizeMove", 1);
}

int OnTimer(void* hWindow, unsigned int nTimerId)
{
    const HWND hWnd = (HWND)hWindow;

    if (nTimerId != SIZEMOVE_TIMER_ID)
    {
        return 0;
    }

    if (s_bInTimerUpdate == 0 && s_bInSizeMove != 0)
    {
        s_bInTimerUpdate = 1;
        GameApp::MainUpdate(hWnd);
        s_bInTimerUpdate = 0;
    }

    return 1;
}

int InSizeMove(void)
{
    return s_bInSizeMove;
}

static void MaskShape(unsigned int nMask, int* pnShift, unsigned int* pnMax)
{
    int nShift = 0;

    if (nMask == 0)
    {
        *pnShift = 0;
        *pnMax   = 1;

        return;
    }

    while ((nMask & 1) == 0)
    {
        nMask >>= 1;
        nShift++;
    }

    *pnShift = nShift;
    *pnMax   = nMask;
}

// Answers -1 when the surface would not lock, 0 when it is all zero, and 1 otherwise.
static int FillFromSurface(IDirectDrawSurface* pSurface, unsigned char* pBits, int nWidth, int nHeight)
{
    DDSURFACEDESC desc;
    HRESULT       hr;
    long          nDibStride;
    int           nAny;
    int           nRShift;
    int           nGShift;
    int           nBShift;
    unsigned int  nRMax;
    unsigned int  nGMax;
    unsigned int  nBMax;
    int           nBpp;
    int           y;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    hr = pSurface->Lock(0, &desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_READONLY, 0);

    if (hr != DD_OK || desc.lpSurface == 0)
    {
        return -1;
    }

    nBpp = desc.ddpfPixelFormat.dwRGBBitCount;

    MaskShape(desc.ddpfPixelFormat.dwRBitMask, &nRShift, &nRMax);
    MaskShape(desc.ddpfPixelFormat.dwGBitMask, &nGShift, &nGMax);
    MaskShape(desc.ddpfPixelFormat.dwBBitMask, &nBShift, &nBMax);

    nDibStride = ((nWidth * 3 + 3) / 4) * 4;
    nAny       = 0;

    for (y = 0; y < nHeight; y++)
    {
        unsigned char* const pSrc = (unsigned char*)desc.lpSurface + (long)y * desc.lPitch;
        unsigned char* const pDst = pBits + (long)(nHeight - 1 - y) * nDibStride;
        int                  x;

        for (x = 0; x < nWidth; x++)
        {
            unsigned int nPixel;

            if (nBpp == 16)
            {
                nPixel = ((unsigned short*)pSrc)[x];
            }
            else if (nBpp == 32)
            {
                nPixel = ((unsigned int*)pSrc)[x];
            }
            else if (nBpp == 24)
            {
                unsigned char* const pThree = pSrc + x * 3;

                nPixel = pThree[0] | (pThree[1] << 8) | (pThree[2] << 16);
            }
            else
            {
                nPixel = pSrc[x];
            }

            if (nPixel != 0)
            {
                nAny = 1;
            }

            pDst[x * 3 + 0] = (unsigned char)(((nPixel >> nBShift) & nBMax) * 255 / nBMax);
            pDst[x * 3 + 1] = (unsigned char)(((nPixel >> nGShift) & nGMax) * 255 / nGMax);
            pDst[x * 3 + 2] = (unsigned char)(((nPixel >> nRShift) & nRMax) * 255 / nRMax);
        }
    }

    pSurface->Unlock(desc.lpSurface);

    return nAny;
}

// GDI reads these surfaces back as zeros, so the pixels come from a lock.
void SaveD3DFrame(const char* pszWhat)
{
    static int s_bSaid = 0;

    BITMAPINFO info;
    HBITMAP    hbm;
    void*      pBits;
    HDC        hdcScreen;
    int        nFilled;
    const int  nWidth  = g_displayMode.nWidth;
    const int  nHeight = g_displayMode.nHeight;

    if (nWidth <= 0 || nHeight <= 0)
    {
        return;
    }

    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize        = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth       = nWidth;
    info.bmiHeader.biHeight      = nHeight;
    info.bmiHeader.biPlanes      = 1;
    info.bmiHeader.biBitCount    = 24;
    info.bmiHeader.biCompression = BI_RGB;

    hdcScreen = GetDC(0);

    if (hdcScreen == 0)
    {
        return;
    }

    pBits = 0;
    hbm   = CreateDIBSection(hdcScreen, &info, DIB_RGB_COLORS, &pBits, 0, 0);

    ReleaseDC(0, hdcScreen);

    if (hbm == 0 || pBits == 0)
    {
        return;
    }

    nFilled = -1;

    if (g_ddrawDevice.pBackSurface != 0)
    {
        nFilled = FillFromSurface(g_ddrawDevice.pBackSurface, (unsigned char*)pBits, nWidth, nHeight);
    }

    if (nFilled < 1 && g_ddrawDevice.pDDrawPrimary != 0)
    {
        const int nPrimary = FillFromSurface(g_ddrawDevice.pDDrawPrimary, (unsigned char*)pBits, nWidth, nHeight);

        if (nPrimary > nFilled)
        {
            nFilled = nPrimary;
        }
    }

    if (s_bSaid == 0)
    {
        s_bSaid = 1;

        Trace("d3dSnapFilled", nFilled);
        Trace("d3dSnapSize", nWidth * 65536 + nHeight);
    }

    if (nFilled >= 0)
    {
        SaveBitmap(pszWhat, hbm);
    }

    DeleteObject(hbm);
}

// HB_SLOW_MS=N, to compare the two cameras at equal speed rather than equal frame count.
void SlowFrame(void)
{
    static int s_nMs = -1;

    if (s_nMs == -1)
    {
        s_nMs = EnvInt("HB_SLOW_MS", 0);
    }

    if (s_nMs > 0)
    {
        Sleep((DWORD)s_nMs);
    }
}

void TraceCamera(void)
{
    static int s_bOn    = -1;
    static int s_nFrame = 0;

    EngineObject* pCam;
    PhysicsBody*  pBody;

    if (s_bOn == -1)
    {
        const char* pszOn = getenv("HB_TRACE_CAM");

        s_bOn = pszOn != 0 && atoi(pszOn) != 0;
    }

    s_nFrame++;

    if (s_bOn == 0 || g_pGameEngine == 0)
    {
        return;
    }

    pCam = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT];

    if (pCam == 0)
    {
        return;
    }

    pBody = pCam->GetPhysicsBody();

    if (pBody == 0)
    {
        return;
    }

    Trace("camFrame", s_nFrame);
    TraceVec3("camAt", pBody->pos.x, pBody->pos.y, pBody->pos.z);
    Trace("camTick", g_pGameEngine->m_clock.nTickMs);
    TraceVec3("camVel", pBody->vel.x, pBody->vel.y, pBody->vel.z);
}

// HB_RES=1280x720. The menus stay at fixed coordinates.
void SelectResolution(void)
{
    const char* pszRes = getenv("HB_RES");

    int nWidth  = 0;
    int nHeight = 0;

    if (pszRes == 0 || sscanf(pszRes, "%dx%d", &nWidth, &nHeight) != 2)
    {
        return;
    }

    if (nWidth < 320 || nHeight < 200 || nWidth > 4096 || nHeight > 2160)
    {
        Trace("resRefused", nWidth);

        return;
    }

    g_dwDisplayWidth  = (unsigned int)nWidth;
    g_dwDisplayHeight = (unsigned int)nHeight;

    Trace("resWidth", nWidth);
    Trace("resHeight", nHeight);
}

// The Direct3D path takes its mode from the enumeration, which overwrites HB_RES.
void OverrideDisplayMode(int* pnWidth, int* pnHeight)
{
    if (getenv("HB_RES") == 0 || pnWidth == 0 || pnHeight == 0)
    {
        return;
    }

    *pnWidth  = (int)g_dwDisplayWidth;
    *pnHeight = (int)g_dwDisplayHeight;

    Trace("d3dModeForced", *pnWidth);
}

// An unaware process is DPI-virtualised, and the menu capture would read stretched pixels.
void BeforeWindow(void)
{
    typedef int(WINAPI * SetDpiAwareFn)(void);

    HMODULE       hUser = GetModuleHandleA("user32.dll");
    SetDpiAwareFn pfnSetAware;

    if (hUser == 0)
    {
        return;
    }

    pfnSetAware = (SetDpiAwareFn)GetProcAddress(hUser, "SetProcessDPIAware");

    if (pfnSetAware == 0)
    {
        Trace("dpiAwareUnavailable", 0);

        return;
    }

    Trace("dpiAware", pfnSetAware());
}

// HB_TOPMOST=1, because the menu capture reads the screen.
void RaiseWindow(void)
{
    static int s_nRaise = -1;

    if (s_nRaise == -1)
    {
        s_nRaise = EnvInt("HB_TOPMOST", 0);
    }

    if (s_nRaise == 0 || g_gameApp.m_hWnd == 0)
    {
        return;
    }

    s_nRaise = 0;

    SetWindowPos(g_gameApp.m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    Trace("windowRaised", (int)g_gameApp.m_hWnd);
}

// HB_WINDOW_STRESS=1 moves, resizes, minimises and unfocuses the window every forty frames.
void StressWindow(void)
{
    static int s_bOn    = -1;
    static int s_nStep  = 0;
    static int s_nFrame = 0;

    HWND hWnd;

    if (s_bOn == -1)
    {
        const char* pszOn = getenv("HB_WINDOW_STRESS");

        s_bOn = 0;

        if (pszOn != 0 && atoi(pszOn) != 0)
        {
            s_bOn = 1;
        }
    }

    hWnd = g_gameApp.m_hWnd;

    if (s_bOn == 0 || hWnd == 0)
    {
        return;
    }

    s_nFrame++;

    if ((s_nFrame % 40) != 0 || s_nStep > 7)
    {
        return;
    }

    s_nStep++;

    Trace("stressStep", s_nStep);

    switch (s_nStep)
    {
        case 1:
            SetWindowPos(hWnd, 0, 60, 40, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            break;

        case 2:
            SetWindowPos(hWnd, 0, 0, 0, 800, 620, SWP_NOMOVE | SWP_NOZORDER);
            break;

        case 3:
            ShowWindow(hWnd, SW_MINIMIZE);
            break;

        case 4:
            ShowWindow(hWnd, SW_RESTORE);
            break;

        case 5:
            ShowWindow(hWnd, SW_MAXIMIZE);
            break;

        case 6:
            ShowWindow(hWnd, SW_RESTORE);
            break;

        case 7:
            SendMessageA(hWnd, WM_ACTIVATEAPP, 0, 0);
            break;

        case 8:
            SendMessageA(hWnd, WM_ACTIVATEAPP, 1, 0);
            break;
    }

    Trace("stressDone", s_nStep);
    Trace("stressAlive", IsWindow(hWnd) != 0);
}

// Answers yes to the quit box; the taunt is still loaded from the pool.
int ConfirmQuit(void)
{
    Trace("quitConfirmSkipped", 1);

    return 1;
}

// Name, position and velocity per object, every HB_TRACE_POS frames.
void TraceScene(GameEngine* pEngine)
{
    static int s_nStride = -1;
    static int s_nFrame  = 0;

    EngineObject** ppObject;
    PhysicsBody*   pBody;
    short          nIndex;

    if (s_nStride == -1)
    {
        const char* pszStride = getenv("HB_TRACE_POS");

        s_nStride = 0;

        if (pszStride != 0)
        {
            s_nStride = atoi(pszStride);
        }
    }

    if (s_nStride <= 0)
    {
        return;
    }

    s_nFrame++;

    if (s_nFrame % s_nStride != 0)
    {
        return;
    }

    BringUp::Trace("sceneFrame", s_nFrame);

    BringUp::Trace("sceneScore1", pEngine->GetHomeScore());
    BringUp::Trace("sceneScore2", pEngine->GetAwayScore());

    ppObject = pEngine->m_ppObjects;

    for (nIndex = 0; nIndex < pEngine->m_nObjectCount; nIndex++)
    {
        if (*ppObject != 0)
        {
            pBody = (*ppObject)->GetPhysicsBody();

            BringUp::Trace("sceneSlot", (int)(*ppObject)->GetSlot());
            BringUp::TraceText("scenePosName", (*ppObject)->m_pszName);
            BringUp::TraceVec3("scenePos", pBody->pos.x, pBody->pos.y, pBody->pos.z);
            BringUp::TraceVec3("sceneVel", pBody->vel.x, pBody->vel.y, pBody->vel.z);
        }

        ppObject++;
    }

    // The camera is not in the object table.
    if (g_pScene != 0)
    {
        pBody = g_pScene->GetPhysicsBody();

        if (pBody != 0)
        {
            BringUp::Trace("sceneCameraTracks", g_pScene->m_nPrimaryTracked);
            BringUp::TraceVec3("sceneCamera", pBody->pos.x, pBody->pos.y, pBody->pos.z);
            BringUp::Trace("sceneCameraSlot", g_pScene->m_nSlot);

            {
                int nSlot;

                for (nSlot = 0; nSlot < 32; nSlot++)
                {
                    CameraObject* const pCam = (CameraObject*)g_pGameEngine->m_ppSlots[nSlot];
                    PhysicsBody*        pCamBody;

                    if (pCam == 0 || pCam->GetSlot() != g_pScene->GetSlot())
                    {
                        continue;
                    }

                    pCamBody = pCam->GetPhysicsBody();

                    if (pCamBody == 0)
                    {
                        continue;
                    }

                    BringUp::Trace("camSlot", nSlot);
                    BringUp::TraceVec3("camPos", pCamBody->pos.x, pCamBody->pos.y, pCamBody->pos.z);
                    {
                        EngineObject* pTrack    = pCam->GetTrackedObject();
                        const char*   pszTracks = "none";

                        if (pTrack != 0)
                        {
                            pszTracks = pTrack->m_pszName;
                        }

                        BringUp::TraceText("camTracks", pszTracks);

                        if (pTrack != 0 && pTrack->GetPhysicsBody() != 0)
                        {
                            BringUp::TraceVec3("camTrackPos",
                                               pTrack->GetPhysicsBody()->pos.x,
                                               pTrack->GetPhysicsBody()->pos.y,
                                               pTrack->GetPhysicsBody()->pos.z);
                        }
                    }

                    if (pCam->m_pSettings != 0)
                    {
                        BringUp::TraceFloat("camSet0", pCam->m_pSettings->x);
                        BringUp::TraceFloat("camSet1", pCam->m_pSettings->y);
                        BringUp::TraceFloat("camSet2", pCam->m_pSettings->z);
                    }

                    BringUp::Trace("camRing", pCam->m_wPosCount);
                    BringUp::TraceFloat("camWeight", pCam->m_flPosWeight);
                    BringUp::TraceVec3("camOut", pCam->m_posOut.x, pCam->m_posOut.y, pCam->m_posOut.z);
                    BringUp::TraceVec3("camWant",
                                       pCam->m_pBody->pos.x,
                                       pCam->m_pBody->pos.y,
                                       pCam->m_pBody->pos.z);
                }
            }
        }
    }
}

// HB_MODELS=shd or txt pins the model set; the renderers read different ones.
void ForceModelSet(int* pbTexEnabled)
{
    const char* pszSet = getenv("HB_MODELS");

    if (pszSet == 0)
    {
        return;
    }

    *pbTexEnabled = (*pszSet == 't');

    Trace("modelsForced", *pbTexEnabled);
}

// Windowed, as -w makes it: full screen leaves the desktop at 640x480.
void ChooseDisplay(void)
{
    g_bFullScreen = 0;

    SelectResolution();
}

void BeforeFrame(void)
{
    HoldActive();
    RaiseWindow();
    StressWindow();
}

void AfterFrame(void* hWindow)
{
    // Nothing in the original waits for the frame.
    PaceFrame();

    SaveBitmap("frame", g_hSoftBackBitmap);

    HoldKeys();
    SendInputActions();
    SendClicks(hWindow);
    SendWidgets();
    DumpWidgets();
    TraceCamera();
    SlowFrame();
    WalkStates();
    ShortenPeriod();
    QuitWhenBudgetSpent();
}

// HB_OVERTIME_MS shortens the run-on after a period for testing; a larger value is ignored.
void ShortenOvertime(int* pnOvertimeMs)
{
    static int s_nOvertimeMs = -1;

    if (s_nOvertimeMs == -1)
    {
        const char* pszMs = getenv("HB_OVERTIME_MS");

        s_nOvertimeMs = 0;

        if (pszMs != 0)
        {
            s_nOvertimeMs = atoi(pszMs);

            if (s_nOvertimeMs < 0)
            {
                s_nOvertimeMs = 0;
            }
        }

        Trace("overtimeMsOverride", s_nOvertimeMs);
    }

    if (s_nOvertimeMs > 0 && s_nOvertimeMs < *pnOvertimeMs)
    {
        *pnOvertimeMs = s_nOvertimeMs;
    }
}

// Scales each 8-bit channel of a COLORREF into its mask.
unsigned int ColourFromMasks(unsigned int nColour, unsigned int nRMask, unsigned int nGMask, unsigned int nBMask)
{
    const unsigned int anChannel[3] = {
        nColour & 0xff, (nColour >> 8) & 0xff, (nColour >> 16) & 0xff
    };
    const unsigned int anMask[3] = { nRMask, nGMask, nBMask };

    unsigned int nValue = 0;
    int          i;

    for (i = 0; i < 3; i++)
    {
        unsigned int nMask  = anMask[i];
        int          nShift = 0;

        if (nMask == 0)
        {
            continue;
        }

        while ((nMask & 1) == 0)
        {
            nMask >>= 1;
            nShift++;
        }

        nValue |= (((anChannel[i] * nMask) / 255) << nShift);
    }

    return nValue;
}

// The surface's top-left pixel read through GDI, in the surface's format; zero when unreadable.
unsigned int CornerColourKey(IDirectDrawSurface* pSurface, unsigned int nRMask, unsigned int nGMask, unsigned int nBMask)
{
    HDC          hdcCorner;
    COLORREF     nCorner;
    unsigned int nValue;

    if (pSurface->GetDC(&hdcCorner) != 0)
    {
        return 0;
    }

    nCorner = GetPixel(hdcCorner, 0, 0);

    pSurface->ReleaseDC(hdcCorner);

    if (nCorner == CLR_INVALID)
    {
        return 0;
    }

    nValue = ColourFromMasks(nCorner, nRMask, nGMask, nBMask);

    Trace("bitmapKeyFromDC", (int)nValue);

    return nValue;
}

// The software palette entry nearest a 0xRRGGBB colour.
int NearestSoftPaletteIndex(unsigned int nColour)
{
    const int rTarget = (nColour >> 16) & 0xff;
    const int gTarget = (nColour >> 8) & 0xff;
    const int bTarget = nColour & 0xff;

    int  nBest     = 0;
    long nBestDist = 0x7fffffff;
    int  i;

    for (i = 0; i < 256; i++)
    {
        const int  dr    = (int)g_aSoftTexPalette[i].peRed - rTarget;
        const int  dg    = (int)g_aSoftTexPalette[i].peGreen - gTarget;
        const int  db    = (int)g_aSoftTexPalette[i].peBlue - bTarget;
        const long nDist = dr * dr + dg * dg + db * db;

        if (nDist < nBestDist)
        {
            nBestDist = nDist;
            nBest     = i;
        }
    }

    return nBest;
}

// Fewer than three vertices, or one at or behind the eye.
int FaceUnprojectable(void)
{
    int v;

    if (g_swFaceBuffer.nCount < 3)
    {
        return 1;
    }

    for (v = 0; v < g_swFaceBuffer.nCount; v++)
    {
        if (g_swFaceBuffer.aVertices[v].pos.w <= 0.0001f)
        {
            return 1;
        }
    }

    return 0;
}

// Takes the two rounded row coordinates, whose low bits are the row once the bias is added.
int EdgeOffScreen(float flRoundY1, float flRoundY2)
{
    const int nRow1 = (short)((int)FloatBits(flRoundY1) + g_nViewportOriginY);
    const int nRow2 = (short)((int)FloatBits(flRoundY2) + g_nViewportOriginY);

    if (nRow1 < 0 || nRow1 >= g_nPrevHeight || nRow2 < 0 || nRow2 >= g_nPrevHeight)
    {
        return 1;
    }

    return 0;
}

static int TraceMessageWanted(int nKind)
{
    static int s_nWanted = -1;

    if (s_nWanted == -1)
    {
        const char* pszWanted = getenv("HB_TRACE_MSG");

        s_nWanted = -2;

        if (pszWanted != 0)
        {
            s_nWanted = atoi(pszWanted);
        }
    }

    return nKind == s_nWanted;
}

// HB_TRACE_MSG names one scene message kind, followed through its first forty posts.
void TraceMessagePosted(const char* pszTo, int nKind, int nWanted)
{
    static int s_nSeen = 0;

    if (TraceMessageWanted(nKind) == 0 || s_nSeen >= 40)
    {
        return;
    }

    s_nSeen++;

    TraceText("msgTo", pszTo);
    Trace("msgKind", nKind);
    Trace("msgWanted", nWanted);
}

// The same kind, through its first forty dispatches.
void TraceMessageDispatched(const char* pszTo, int nKind, int nSlot, int nPhase)
{
    static int s_nSeen = 0;

    if (TraceMessageWanted(nKind) == 0 || s_nSeen >= 40)
    {
        return;
    }

    s_nSeen++;

    TraceText("msgDispatchTo", pszTo);
    Trace("msgDispatchSlot", nSlot);
    Trace("msgDispatchPhase", nPhase);
}

// HB_TRACE_OBJ names one object, whose timers are traced for their first two hundred steps.
void TraceTimerStep(const char* pszOwner, int bDead, int nDeadline, int nMessage)
{
    static const char* s_pszWanted = (const char*)1;
    static int         s_nSteps    = 0;

    if (s_pszWanted == (const char*)1)
    {
        s_pszWanted = getenv("HB_TRACE_OBJ");
    }

    if (s_pszWanted == 0 || pszOwner == 0 || strcmp(pszOwner, s_pszWanted) != 0 || s_nSteps >= 200)
    {
        return;
    }

    s_nSteps++;

    Trace("timerStepDead", bDead);
    Trace("timerStepNow", (int)g_pGameEngine->m_clock.nTickMs);
    Trace("timerStepDeadline", nDeadline);
    Trace("timerStepMsg", nMessage);
}

} // namespace BringUp

#endif
