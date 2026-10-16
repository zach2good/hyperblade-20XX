#ifndef HYPERBLADE_NETWORK_H
#define HYPERBLADE_NETWORK_H

#include "common/compat.h"

struct GameSession;
struct SyncBlockHeader;
struct PlayerSlotName;

namespace Network
{

int            ShowWaitingDialog(void);
void           FlushPendingPlayerName(void);
int __fastcall AddDPID(struct GameSession* pSession, DWORD nId);
int __fastcall RemoveDPID(struct GameSession* pSession, DWORD nId);
int __stdcall  ValidateStaticObjects(void);
unsigned int   SetupPlayerSlot(unsigned int nDpid, int nSlot);
unsigned int   AbortAndCleanup(HWND hWnd);
void __cdecl   SendAssetData(int nRequesterId);
void __cdecl   ReceiveAssetPacket(const SyncBlockHeader* pPacket, unsigned int nLength, int nSenderId);
void __cdecl   OnPlayerConnected(DWORD idFrom, int nSlot);
void           OnAcceptPlayerSlot(int nPlayer);
int __fastcall AddPlayer(struct GameSession* pSession, unsigned int nDpid, PlayerSlotName* pName);
void           OnRejectPlayerSlot(int nPlayer);

} // namespace Network

#endif
