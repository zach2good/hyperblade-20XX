#ifndef HYPERBLADE_GAMEAPP_H
#define HYPERBLADE_GAMEAPP_H

#include "common/compat.h"
#include "globals.h"
#include "common/pathstring.h"
#include "common/stringpool.h"
#include "math/vec3.h"

class Interactor;
struct PlanNode;

struct GameApp
{
    static void __cdecl   ClipCursorToWindow(HWND hWnd);
    static int __cdecl    HandleMatchKeypress(unsigned int nKey);
    static void __cdecl   ShowHelp(HWND hWnd);
    static void __cdecl   HandleGlobalKeyEvent(HWND hWnd, int nKey);
    static void __stdcall OnStartup(void);
    static void __stdcall OnExit(void);

    // Free functions in the original, reached through g_gameApp.
    static int GetInstallLevel(void);
    static int SetCursorVisible(int bVisible);

    int WasCursorVisible() const
    {
        return m_bCursorWasVisible;
    }

    static BOOL CALLBACK  AnimationPeekDialogProc(HWND hDlg, UINT nMsg, UINT wParam, LONG lParam);
    static void           SetupViewports(void);
    static void __stdcall InstantiateState(void);
    static void           ResetToSinglePlayer(void);
    static int __cdecl    OutOfMemoryHandler(unsigned int nSize);
    static void __cdecl   AnimationPeek(Interactor*, PlanNode*, short*, int nWindow);
    static int            ShowMessageBox(HWND hWnd, LPCSTR pszText, LPCSTR pszCaption, UINT nType);
    static LRESULT        ClearSplashPalette(void);
    static unsigned int   RegisterWindowClass(HINSTANCE hInstance, LPCSTR pszClassName);
    static HWND           CreateMainWindow(HINSTANCE hInstance, LPCSTR pszClassName, LPVOID pParam);
    static void __cdecl   Shutdown(HWND hWnd);

    void    ReportLastError(HWND hUnused);
    void    Error(const char* pszFormat, ...);
    void    Fatal(const char* pszFormat, ...);
    void    FatalString(const char* pszFormat, ...);
    void    FatalErrorFmtString(HWND hWnd, const char* pszFormat, va_list args);
    HBITMAP LoadGDIBitmap(const char* pszName);

    int FindCDRomDrive(bool bRequired);

    HWND GetMainWindow() const
    {
        return m_hWnd;
    }

    HINSTANCE GetInstance() const
    {
        return m_hInstance;
    }

    HDC GetScreenDC() const
    {
        return m_hScreenDC;
    }

    HMENU GetMainMenu() const
    {
        return m_hMenu;
    }

    explicit GameApp(short nStringCount);

    // All built from the exe directory; Display::ApplyRenderOptions rebuilds m_strModelPath.
    PathString m_strExeDir;       // 0x00, and the data directory under it
    PathString m_strAssetsPath;   // 0x04, "assets8"
    PathString m_strAnimDataPath; // 0x08, "AnimData"
    PathString m_strModelPath;    // 0x0c
    PathString m_strPlansPath;    // 0x10, "Plans"
    PathString m_strSoundsPath;   // 0x14, "Sounds"
    PathString m_strTexturesPath; // 0x18, "Textures"
    // The rest of the program reaches these fields at their absolute addresses.
    Vec3          m_vLightSpherical; // 0x1c
    int           m_nFrameRate;      // 0x28
    HWND          m_hWnd;            // 0x2c
    HINSTANCE     m_hInstance;       // 0x30
    HPALETTE      m_hPalette;        // 0x34
    HDC           m_hScreenDC;       // 0x38
    HMENU         m_hMenu;           // 0x3c
    unsigned int  m_nEngineState;    // 0x40
    unsigned char m_bIsActive;       // 0x44
    char          m_cUnused;         // 0x45
    // The engine's run state when the window lost focus, restored when it comes back.
    unsigned short m_wRunStateOnDeactivate; // 0x46
    int            m_bCursorWasVisible;     // 0x48
    HWND           m_hWndActiveDialog;      // 0x4c
    // The window class name, which the resource path initialiser loads from the string table.
    String m_strCaption; // 0x50
    // Sixteen bytes, and its string at 0x60 is the one the error reporters format into.
    StringPool m_stringPool;              // 0x54
    void(__stdcall* m_pfnShutdown)(void); // 0x64
    int m_nCDRomDrive;                    // 0x68

    void CopyCDFile(const char* pszName);

    void __fastcall OnShutdown();

    void SetWindowAndCallbacks(HWND hWnd, void(__stdcall* pfnStartup)(void), void(__stdcall* pfnShutdown)(void));

    void FatalErrorFmt(HWND hWnd, const char* pszFormat, va_list args);

    void FatalError(HWND hWnd, String strMessage);
    void ShowErrorBox(HWND hWnd, String strMessage);
    void ErrorFmt(HWND hWnd, const char* pszFormat, va_list args);

    static int MainUpdate(HWND hWnd);

    static HWND  Initialize(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nShowCmd, LPSTR lpCmdLine);
    static void  InitializeResourcePaths(HINSTANCE hInstance);
    static void  ParseCommandLine(LPSTR lpCmdLine);
    static char* ResolveCDRomPath(char* pszPath);

    static void SetModelPath(int bTexEnabled);

    static void __stdcall SplashTimerCallback(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
};

// SYNTHETIC: HYPERX 0x0044d400
// GameApp::~GameApp

#endif
