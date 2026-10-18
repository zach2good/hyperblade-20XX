#include "common/compat.h"
#include <SDL3/SDL.h>
#include "engine/gameoptions.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "graphics/display.h"
#include <cstdlib>
#include <cstring>
#include "app/gameapp.h"
#include "engine/objectclasses.h"
#include "ui/widget.h"
#include "engine/cameraobject.h"
#include "graphics/graphics.h"

// The previous size is latched the first time either half is zero.
// FUNCTION: HYPERX 0x0044b170
void Graphics::Initialize(int bResize)
{
    LayoutState layout;
    int         nHeight;
    int         nPrevHeight;

    if (bResize != 0)
    {
        nHeight = g_nBackBufferHeight;

        if (nHeight != 0)
        {
            g_pLeftStack = static_cast<unsigned short*>(realloc(g_pLeftStack, nHeight * sizeof(unsigned short)));

            if (g_pLeftStack == nullptr)
            {
                // STRING: HYPERX 0x004c2d08
                g_gameApp.Fatal("Insufficient memory for Graphics Render Engine! (left_stack)!");
            }

            g_pRightStack = static_cast<unsigned short*>(realloc(g_pRightStack, g_nBackBufferHeight * sizeof(unsigned short)));

            if (g_pRightStack == nullptr)
            {
                // STRING: HYPERX 0x004c2cc8
                g_gameApp.Fatal("Insufficient memory for Graphics Render Engine! (right_stack)!");
            }
        }
    }

    nHeight = g_nBackBufferHeight;

    g_pScene = nullptr;

    if (g_pGameEngine != nullptr)
    {
        CameraObject* const pCamera = g_pGameEngine->GetCamera(kCameraMainSlot);

        int nPrevWidth = g_nPrevWidth;

        if (nPrevWidth == 0 || (nPrevHeight = g_nPrevHeight) == 0)
        {
            nPrevWidth  = g_nBackBufferWidth;
            nPrevHeight = nHeight;

            g_nPrevWidth  = nPrevWidth;
            g_nPrevHeight = nPrevHeight;
        }

        layout = Widget::InitLayoutState(g_pBackBufferBitsCopy, g_nBackBufferWidth, nHeight, g_nBackBufferWidth, nPrevWidth, nPrevHeight, (g_nBackBufferWidth - nPrevWidth) / 2, (nHeight - nPrevHeight) / 2);

        pCamera->layout = layout;

        pCamera->InitViewTransforms();

        g_pGameEngine->SetScreenSize(g_pGameOptions->ReadScreenSize());
    }
}

// Loads only when the command line carries the developer marker.
// FUNCTION: HYPERX 0x0044b6f0
unsigned int Graphics::LoadMdmTexDLL()
{
    const char* const pszCmdLine = GameApp::Arguments();

    // The original calls _mbsstr, the multibyte wrapper at 0x0049f75c, not the plain routine.
    // STRING: HYPERX 0x004c2da0
    if (strstr(pszCmdLine, "dmandappro!") == nullptr)
    {
        return 0;
    }

    // STRING: HYPERX 0x004c2d94
    SDL_SharedObject* const pLibrary = SDL_LoadObject("mdmtex.dll");

    if (pLibrary == nullptr)
    {
        return 0;
    }

    // STRING: HYPERX 0x004c2d84
    g_pfnMdmInitDrawPoly = reinterpret_cast<MdmInitDrawPolyFn>(SDL_LoadFunction(pLibrary, "mdmInitDrawPoly"));

    // STRING: HYPERX 0x004c2d74
    g_pfnMdmDrawTexPoly = reinterpret_cast<MdmDrawTexPolyFn>(SDL_LoadFunction(pLibrary, "mdmDrawTexPoly"));

    // The original freed the library here and kept its handle, which then read as loaded.
    if (g_pfnMdmInitDrawPoly == nullptr || g_pfnMdmDrawTexPoly == nullptr)
    {
        SDL_UnloadObject(pLibrary);

        return 0;
    }

    g_hMdmTexDll = Handle<struct MdmTexLibraryTag>{ pLibrary };

    atexit(Graphics::UnloadMdmTexDLL);

    return 1;
}

// FUNCTION: HYPERX 0x0044b7a0
void Graphics::UnloadMdmTexDLL()
{
    if (g_hMdmTexDll)
    {
        SDL_UnloadObject(g_hMdmTexDll.As<SDL_SharedObject>());

        g_hMdmTexDll = {};
    }
}
