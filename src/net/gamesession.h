#ifndef HYPERBLADE_GAMESESSION_H
#define HYPERBLADE_GAMESESSION_H

// Every player slot in a session; the DirectPlay session itself allows more.
#define SESSION_PLAYER_COUNT 16

// The first byte of a player's name: the slot in the low nibble and who drives it above. A slot
// the host machine drives carries the host bit, so each side applies only the other side's slots.
#define PLAYER_NAME_SLOT_MASK 0x0f
#define PLAYER_NAME_LOCAL     0x10
#define PLAYER_NAME_HUMAN     0x40
#define PLAYER_NAME_HOST      0x80

#include "common/compat.h"
#include "session.h"
#include "net/directplay.h"

// The application identifier a hosted session is created under.
// GLOBAL: HYPERX 0x004c0ec0
extern const GUID g_guidNetApplication;

struct GameSession
{
    static void __stdcall Disconnect(void);
    static int __cdecl    ValidateAndApplyAssets(struct SyncBlocks* pLocal, struct SyncBlocks* pRemote);

    // The host and join dialog handlers both build one of these.
    GameSession(HWND hDlg, IDirectPlay* pLobby);
    unsigned int CreatePacketCarrier();

    int OpenSession(DWORD dwFlags, const DPSESSIONDESC* pDesc);

    int AssignPlayerSlot(unsigned int nFlags, PlayerSlotName* pName);

    ~GameSession();

    // The other machines' DirectPlay ids; Network::AddDPID and RemoveDPID keep the list.
    DWORD aRemoteIds[SESSION_REMOTE_IDS]; // 0x000
    int   nRemoteCount;                   // 0x020
    // A four-bit mask of the asset blocks that have arrived.
    unsigned int nAssetsArrived; // 0x024
    HWND         hDlg;           // 0x028, the setup dialog the constructor is handed
    IDirectPlay* pDPlay;         // 0x02c
    // Whether this machine hosts; only the host decides every slot is ready.
    int           bHosting;    // 0x030
    DPSESSIONDESC sessionDesc; // 0x034
    // Indexed by network slot; NET_SLOT_GOD queues the referee's events.
    class CPlayer* apPlayers[SESSION_PLAYER_SLOTS]; // 0x0b0
    // The lead player; null fails with "Trying start a game without a Lead player".
    class CPlayer* pLeadPlayer;        // 0x0f0
    int            nPendingDisconnect; // 0x0f4
    // The lag in milliseconds, four at construction.
    unsigned int nLagMs; // 0x0f8

    GameSession* Destroy();

    int PumpNetworkCommands();

    int AreAllSlotsReady();

    void __fastcall HandlePlayerDisconnect(int nIndex);

    // Its body never reads the session.
    void __fastcall ActivatePlayerSlot(int nSlot, unsigned int bHost);

    static int __cdecl BuildSyncPacket(struct SyncBlocks* pBlocks, struct SyncBlocks* pRemoteBlocks);

    static int __cdecl GetFileInfo(const char* pszPath, int* pnWriteTime, int* pnSize);

    static int __cdecl GetPlansEntry(const char* pszFileName, char* pszName, int nWanted);

    static int FreeAssetBuffer(struct SyncBlocks* pCatalog, struct SyncBlocks* pDescriptor);
};

#endif
