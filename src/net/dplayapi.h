// The DirectPlay 1 interface the game uses. The Windows SDK no longer ships dplay.h or
// dplayx.lib, so the entry points are resolved from dplayx.dll at run time.

#ifndef HYPERBLADE_DPLAYAPI_H
#define HYPERBLADE_DPLAYAPI_H

#include <windows.h>
#include <objbase.h>

typedef DWORD DPID;
typedef DPID* LPDPID;

#define DPID_SYSMSG     0
#define DPID_ALLPLAYERS 0
#define DPID_UNKNOWN    0xFFFFFFFF

#define DPLONGNAMELEN    52
#define DPSHORTNAMELEN   20
#define DPSESSIONNAMELEN 32
#define DPPASSWORDLEN    16
#define DPUSERRESERVED   16

#define DPOPEN_JOIN              0x00000001
#define DPOPEN_CREATE            0x00000002
#define DPENUMSESSIONS_AVAILABLE 0x00000001
#define DPESC_TIMEDOUT           0x00000001
#define DPRECEIVE_ALL            0x00000001
#define DPRECEIVE_TOPLAYER       0x00000002
#define DPSEND_GUARANTEED        0x00000001

#define DPSYS_ADDPLAYER           0x0003
#define DPSYS_DELETEPLAYER        0x0005
#define DPSYS_ADDPLAYERTOGROUP    0x0007
#define DPSYS_DELETEGROUP         0x0020
#define DPSYS_DELETEPLAYERFROMGRP 0x0021
#define DPSYS_SESSIONLOST         0x0031
#define DPSYS_CONNECT             0x484b

#define MAKE_DPHRESULT(code) MAKE_HRESULT(1, 0x877, code)

#define DP_OK                     S_OK
#define DPERR_ALREADYINITIALIZED  MAKE_DPHRESULT(5)
#define DPERR_ACCESSDENIED        MAKE_DPHRESULT(10)
#define DPERR_ACTIVEPLAYERS       MAKE_DPHRESULT(20)
#define DPERR_BUFFERTOOSMALL      MAKE_DPHRESULT(30)
#define DPERR_CANTADDPLAYER       MAKE_DPHRESULT(40)
#define DPERR_CANTCREATEGROUP     MAKE_DPHRESULT(50)
#define DPERR_CANTCREATEPLAYER    MAKE_DPHRESULT(60)
#define DPERR_CANTCREATESESSION   MAKE_DPHRESULT(70)
#define DPERR_CAPSNOTAVAILABLEYET MAKE_DPHRESULT(80)
#define DPERR_EXCEPTION           MAKE_DPHRESULT(90)
#define DPERR_GENERIC             E_FAIL
#define DPERR_INVALIDFLAGS        MAKE_DPHRESULT(120)
#define DPERR_INVALIDOBJECT       MAKE_DPHRESULT(130)
#define DPERR_INVALIDPARAM        E_INVALIDARG
#define DPERR_INVALIDPARAMS       DPERR_INVALIDPARAM
#define DPERR_INVALIDPLAYER       MAKE_DPHRESULT(150)
#define DPERR_NOCAPS              MAKE_DPHRESULT(160)
#define DPERR_NOCONNECTION        MAKE_DPHRESULT(170)
#define DPERR_NOMEMORY            E_OUTOFMEMORY
#define DPERR_OUTOFMEMORY         DPERR_NOMEMORY
#define DPERR_NOMESSAGES          MAKE_DPHRESULT(190)
#define DPERR_NONAMESERVERFOUND   MAKE_DPHRESULT(200)
#define DPERR_NOPLAYERS           MAKE_DPHRESULT(210)
#define DPERR_NOSESSIONS          MAKE_DPHRESULT(220)
#define DPERR_SENDTOOBIG          MAKE_DPHRESULT(230)
#define DPERR_TIMEOUT             MAKE_DPHRESULT(240)
#define DPERR_UNAVAILABLE         MAKE_DPHRESULT(250)
#define DPERR_UNSUPPORTED         E_NOTIMPL
#define DPERR_BUSY                MAKE_DPHRESULT(270)
#define DPERR_USERCANCEL          MAKE_DPHRESULT(280)

struct DPCAPS
{
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwMaxBufferSize;
    DWORD dwMaxQueueSize;
    DWORD dwMaxPlayers;
    DWORD dwHundredBaud;
    DWORD dwLatency;
    DWORD dwMaxLocalPlayers;
    DWORD dwHeaderLength;
    DWORD dwTimeout;
};

typedef DPCAPS* LPDPCAPS;

struct DPSESSIONDESC
{
    DWORD dwSize;
    GUID  guidSession;
    DWORD dwSession;
    DWORD dwMaxPlayers;
    DWORD dwCurrentPlayers;
    DWORD dwFlags;
    char  szSessionName[DPSESSIONNAMELEN];
    char  szUserField[DPUSERRESERVED];
    DWORD dwReserved1;
    char  szPassword[DPPASSWORDLEN];
    DWORD dwReserved2;
    DWORD dwUser1;
    DWORD dwUser2;
    DWORD dwUser3;
    DWORD dwUser4;
};

typedef DPSESSIONDESC* LPDPSESSIONDESC;

static_assert(sizeof(DPSESSIONDESC) == 0x7c);

struct DPMSG_GENERIC
{
    DWORD dwType;
};

struct DPMSG_ADDPLAYER
{
    DWORD dwType;
    DWORD dwPlayerType;
    DPID  dpId;
    char  szLongName[DPLONGNAMELEN];
    char  szShortName[DPSHORTNAMELEN];
    DWORD dwCurrentPlayers;
};

struct DPMSG_DELETEPLAYER
{
    DWORD dwType;
    DPID  dpId;
};

typedef BOOL(PASCAL* LPDPENUMPLAYERSCALLBACK)(DPID dpId, LPSTR lpFriendlyName, LPSTR lpFormalName, DWORD dwFlags, LPVOID lpContext);
typedef BOOL(PASCAL* LPDPENUMSESSIONSCALLBACK)(LPDPSESSIONDESC lpDPSessionDesc, LPVOID lpContext, LPDWORD lpdwTimeOut, DWORD dwFlags);
typedef BOOL(PASCAL* LPDPENUMDPCALLBACKA)(LPGUID lpguidSP, LPSTR lpSPName, DWORD dwMajorVersion, DWORD dwMinorVersion, LPVOID lpContext);

struct IDirectPlay : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE AddPlayerToGroup(DPID, DPID)                                                  = 0;
    virtual HRESULT STDMETHODCALLTYPE Close()                                                                       = 0;
    virtual HRESULT STDMETHODCALLTYPE CreatePlayer(LPDPID, LPSTR, LPSTR, LPHANDLE)                                  = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateGroup(LPDPID, LPSTR, LPSTR)                                             = 0;
    virtual HRESULT STDMETHODCALLTYPE DeletePlayerFromGroup(DPID, DPID)                                             = 0;
    virtual HRESULT STDMETHODCALLTYPE DestroyPlayer(DPID)                                                           = 0;
    virtual HRESULT STDMETHODCALLTYPE DestroyGroup(DPID)                                                            = 0;
    virtual HRESULT STDMETHODCALLTYPE EnableNewPlayers(BOOL)                                                        = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumGroupPlayers(DPID, LPDPENUMPLAYERSCALLBACK, LPVOID, DWORD)                = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumGroups(DWORD, LPDPENUMPLAYERSCALLBACK, LPVOID, DWORD)                     = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumPlayers(DWORD, LPDPENUMPLAYERSCALLBACK, LPVOID, DWORD)                    = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumSessions(LPDPSESSIONDESC, DWORD, LPDPENUMSESSIONSCALLBACK, LPVOID, DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCaps(LPDPCAPS)                                                             = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMessageCount(DPID, LPDWORD)                                                = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPlayerCaps(DPID, LPDPCAPS)                                                 = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPlayerName(DPID, LPSTR, LPDWORD, LPSTR, LPDWORD)                           = 0;
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPGUID)                                                            = 0;
    virtual HRESULT STDMETHODCALLTYPE Open(LPDPSESSIONDESC)                                                         = 0;
    virtual HRESULT STDMETHODCALLTYPE Receive(LPDPID, LPDPID, DWORD, LPVOID, LPDWORD)                               = 0;
    virtual HRESULT STDMETHODCALLTYPE SaveSession(LPSTR)                                                            = 0;
    virtual HRESULT STDMETHODCALLTYPE Send(DPID, DPID, DWORD, LPVOID, DWORD)                                        = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPlayerName(DPID, LPSTR, LPSTR)                                             = 0;
};

typedef IDirectPlay* LPDIRECTPLAY;

HRESULT WINAPI DirectPlayEnumerateA(LPDPENUMDPCALLBACKA pCallback, LPVOID pContext);
HRESULT WINAPI DirectPlayCreate(LPGUID pGuid, LPDIRECTPLAY* ppDirectPlay, IUnknown* pOuter);

#endif
