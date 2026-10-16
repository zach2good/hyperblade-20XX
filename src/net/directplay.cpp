#include "common/compat.h"
#include "globals.h"
#include "netglobals.h"
#include "session.h"
#include "directplay.h"

#define DPLAY_ERROR_COUNT 31

// Null when dplayx.dll is missing, which the callers already treat as no DirectPlay.
static FARPROC DPlayEntry(const char* pszName)
{
    static HMODULE s_hDPlay = LoadLibraryA("dplayx.dll");

    if (s_hDPlay == 0)
    {
        return 0;
    }

    return GetProcAddress(s_hDPlay, pszName);
}

HRESULT WINAPI DirectPlayEnumerateA(LPDPENUMDPCALLBACKA pCallback, LPVOID pContext)
{
    typedef HRESULT(WINAPI * EnumerateFn)(LPDPENUMDPCALLBACKA, LPVOID);
    const EnumerateFn pfnEnumerate = (EnumerateFn)DPlayEntry("DirectPlayEnumerateA");

    if (pfnEnumerate == 0)
    {
        return DPERR_UNAVAILABLE;
    }

    return pfnEnumerate(pCallback, pContext);
}

HRESULT WINAPI DirectPlayCreate(LPGUID pGuid, LPDIRECTPLAY* ppDirectPlay, IUnknown* pOuter)
{
    typedef HRESULT(WINAPI * CreateFn)(LPGUID, LPDIRECTPLAY*, IUnknown*);
    const CreateFn pfnCreate = (CreateFn)DPlayEntry("DirectPlayCreate");

    if (pfnCreate == 0)
    {
        return DPERR_UNAVAILABLE;
    }

    return pfnCreate(pGuid, ppDirectPlay, pOuter);
}

// An unknown code returns a fixed string rather than a null.
// FUNCTION: HYPERX 0x00442880
char* DirectPlay::GetErrorString(int hr)
{
    // STRING: HYPERX 0x004c1e48
    char* pszText = "unknown error";

    for (unsigned int nIndex = 0; nIndex < DPLAY_ERROR_COUNT; nIndex++)
    {
        if (g_aDPlayErrors[nIndex].nCode == hr)
        {
            pszText = g_aDPlayErrors[nIndex].pszText;
            break;
        }
    }

    return pszText;
}

DPlayError g_aDPlayErrors[DPLAY_ERROR_COUNT] = {
    { DP_OK, "DP_OK" },
    { DPERR_ALREADYINITIALIZED, "DPERR_ALREADYINITIALIZED" },
    { DPERR_ACCESSDENIED, "DPERR_ACCESSDENIED" },
    { DPERR_ACTIVEPLAYERS, "DPERR_ACTIVEPLAYERS" },
    { DPERR_BUFFERTOOSMALL, "DPERR_BUFFERTOOSMALL" },
    { DPERR_CANTADDPLAYER, "DPERR_CANTADDPLAYER" },
    { DPERR_CANTCREATEGROUP, "DPERR_CANTCREATEGROUP" },
    { DPERR_CANTCREATEPLAYER, "DPERR_CANTCREATEPLAYER" },
    { DPERR_CANTCREATESESSION, "DPERR_CANTCREATESESSION" },
    { DPERR_CAPSNOTAVAILABLEYET, "DPERR_CAPSNOTAVAILABLEYET" },
    { DPERR_EXCEPTION, "DPERR_EXCEPTION" },
    { DPERR_GENERIC, "DPERR_GENERIC" },
    { DPERR_INVALIDFLAGS, "DPERR_INVALIDFLAGS" },
    { DPERR_INVALIDOBJECT, "DPERR_INVALIDOBJECT" },
    { DPERR_INVALIDPARAM, "DPERR_INVALIDPARAM" },
    { DPERR_INVALIDPARAMS, "DPERR_INVALIDPARAMS" },
    { DPERR_INVALIDPLAYER, "DPERR_INVALIDPLAYER" },
    { DPERR_NOCAPS, "DPERR_NOCAPS" },
    { DPERR_NOCONNECTION, "DPERR_NOCONNECTION" },
    { DPERR_NOMEMORY, "DPERR_NOMEMORY" },
    { DPERR_OUTOFMEMORY, "DPERR_OUTOFMEMORY" },
    { DPERR_NOMESSAGES, "DPERR_NOMESSAGES" },
    { DPERR_NONAMESERVERFOUND, "DPERR_NONAMESERVERFOUND" },
    { DPERR_NOPLAYERS, "DPERR_NOPLAYERS" },
    { DPERR_NOSESSIONS, "DPERR_NOSESSIONS" },
    { DPERR_SENDTOOBIG, "DPERR_SENDTOOBIG" },
    { DPERR_TIMEOUT, "DPERR_TIMEOUT" },
    { DPERR_UNAVAILABLE, "DPERR_UNAVAILABLE" },
    { DPERR_UNSUPPORTED, "DPERR_UNSUPPORTED" },
    { DPERR_BUSY, "DPERR_BUSY" },
    // STRING: HYPERX 0x004c1350
    { DPERR_USERCANCEL, "DPERR_USERCANCEL" }
};
