#ifndef HYPERBLADE_GAMEAPP_H
#define HYPERBLADE_GAMEAPP_H

#include "common/compat.h"
#include "app/window.h"
#include "globals.h"
#include "common/pathstring.h"
#include "common/stringpool.h"
#include "math/vec3.h"

class Interactor;
struct PlanNode;

struct GameApp
{
    static int  HandleMatchKeypress(unsigned int nKey);
    static void HandleGlobalKeyEvent(int nKey);
    static void OnStartup();
    static void OnExit();

    // Free functions in the original, reached through g_gameApp.
    static int GetInstallLevel();
    static int SetCursorVisible(int bVisible);

    int WasCursorVisible() const
    {
        return m_bCursorWasVisible;
    }

    static void                  SetupViewports();
    static void                  InstantiateState();
    static void                  ResetToSinglePlayer();
    static void                  OutOfMemoryHandler();
    static void                  AnimationPeek(Interactor*, PlanNode*, short*, int nWindow);
    static Window::MessageAnswer ShowMessageBox(const char* pszText, const char* pszCaption, Window::MessageButtons buttons);
    static bool                  CreateMainWindow(const char* pszTitle);
    static void                  Shutdown();

    void ReportLastError();
    void Error(const char* pszFormat, ...);
    void Fatal(const char* pszFormat, ...);
    void FatalString(const char* pszFormat, ...);
    void FatalErrorFmtString(const char* pszFormat, va_list args);

    int FindCDRomDrive(bool bRequired);

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
    bool          m_bHasWindow;      // 0x2c
    unsigned int  m_nEngineState;    // 0x40
    unsigned char m_bIsActive;       // 0x44
    char          m_cUnused;         // 0x45
    // The engine's run state when the window lost focus, restored when it comes back.
    unsigned short m_wRunStateOnDeactivate; // 0x46
    int            m_bCursorWasVisible;     // 0x48
    // The window class name, which the resource path initialiser loads from the string table.
    String m_strCaption; // 0x50
    // Sixteen bytes, and its string at 0x60 is the one the error reporters format into.
    StringPool m_stringPool; // 0x54
    void (*m_pfnShutdown)(); // 0x64
    int m_nCDRomDrive;       // 0x68

    void OnShutdown();

    void SetWindowAndCallbacks(bool bHasWindow, void (*pfnStartup)(), void (*pfnShutdown)());

    void FatalErrorFmt(const char* pszFormat, va_list args);

    void FatalError(String strMessage);
    void ShowErrorBox(String strMessage);
    void ErrorFmt(const char* pszFormat, va_list args);

    static int MainUpdate();

    static bool Initialize(const char* pszArguments);

    // The command line after the program's name.
    static const char* Arguments();
    static void        InitializeResourcePaths();
    static void        ParseCommandLine(const char* pszArguments);
    static char*       ResolveCDRomPath(char* pszPath);

    static void SetModelPath(int bTexEnabled);

    // The splashes run in the main loop before the menus, and a key or click skips each one.
    static void NextSplash();
    static bool UpdateSplash();
};

// SYNTHETIC: HYPERX 0x0044d400
// GameApp::~GameApp

#endif
